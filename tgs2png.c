#ifndef _WIN32
#include <libgen.h>
#else
#include <windows.h>
#endif  /* _WIN32 */

#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <rlottie_capi.h>
#include <png.h>

const char* version = "0.2.0";

static bool debug = false;
static bool show_info = false;
static int start_frame = 0;
static int nframes = -1;
static float scale = 1.0;
static int loop = 1;
static int width = 0;
static int height = 0;
//static float speed = 1.0;

static int compress_level = 1;

struct tgs_png {
        png_bytep buffer;        /* pixel data */
        png_bytep* rows;         /* rows pointing inside buffer */

        /* Buffer holding PNG image data, used at render time */
        size_t data_size;
        size_t data_off;
        uint8_t* data;

        /* Last rendered frame number, for debugging */
        size_t frame_num;

        /*
         * timeval of last flush
         * Used to calculate sleeping time
         */
        int start_frame;
        double fps;
        struct timeval start_time;
};

static void
tgs_png_error_fn(png_structp ctx, png_const_charp str)
{
        perror(str);
}

static void
tgs_png_write_fn(png_structp ctx, png_bytep data, png_size_t length)
{
        struct tgs_png* tgs = (struct tgs_png*)png_get_io_ptr(ctx);
        assert(tgs != NULL);
        if (tgs->data_off + length > tgs->data_size) {
                tgs->data_size += tgs->data_size;
                tgs->data = realloc(tgs->data, tgs->data_size);
                assert(tgs->data != NULL);
        }
        memcpy(&tgs->data[tgs->data_off], data, length);
        tgs->data_off += length;
}

void
tgs_png_init(struct tgs_png* tgs)
{
        memset(tgs, '\0', sizeof(struct tgs_png));
        tgs->buffer = (png_bytep)malloc(width * height * 4);
        assert(tgs->buffer != NULL);
        tgs->rows = (png_bytep*)malloc(sizeof(png_bytep) * height);
        assert(tgs->rows != NULL);
        for (int i = 0; i < height; i++)
                tgs->rows[i] = &tgs->buffer[i * width * 4];

        tgs->data_size = width * height * 4;
        tgs->data_off = 0;
        tgs->data = malloc(tgs->data_size);
        assert(tgs->data != NULL);
}

void
tgs_png_fini(struct tgs_png* tgs)
{
        if (tgs->buffer != NULL)
                free(tgs->buffer);
        if (tgs->rows != NULL)
                free(tgs->rows);
        if (tgs->data != NULL)
                free(tgs->data);
}

void
tgs_png_timer_init(struct tgs_png* tgs, int start_frame, double fps)
{
        tgs->start_frame = start_frame;
        tgs->fps = fps;
        if (gettimeofday(&tgs->start_time, NULL))
                assert(false);
}

void
tgs_png_timer_sleep(struct tgs_png* tgs, int frame)
{
        assert(frame > tgs->start_frame);

        long frame_usec = (1000000 * (frame - tgs->start_frame)) / tgs->fps;
        struct timeval frame_time;
        frame_time.tv_sec = frame_usec / 1000000;
        frame_time.tv_usec = (frame_usec
                              - (frame_time.tv_sec * 1000000));

        timeradd(&tgs->start_time, &frame_time, &frame_time);

        struct timeval now_time;
        if (gettimeofday(&now_time, NULL))
                assert(false);

        if (timercmp(&frame_time, &now_time, >)) {
                struct timeval frame_timeout;
                timersub(&frame_time, &now_time, &frame_timeout);

                struct timespec sleep_time = {
                        .tv_sec = frame_timeout.tv_sec,
                        .tv_nsec = frame_timeout.tv_usec * 1000} ;
                if (debug) {
                        printf("Sleeping %lu nsec, frame=%d\n",
                               sleep_time.tv_nsec, frame);
                }
                while (nanosleep(&sleep_time, &sleep_time) == EINTR)
                        ;
        }
}

void
tgs_png_flush(struct tgs_png* tgs)
{
        if (debug) {
                printf("PNG frame=%zu, size=%zu\n",
                       tgs->frame_num, tgs->data_off);
        } else {
                /* Write PNG data to stdout */
                ssize_t woff = 0;
                size_t wb;
                size_t write_sz = tgs->data_off;
                do {
                        wb = write(1, &tgs->data[woff], write_sz);
                        if (wb <= 0)
                                break;
                        assert(wb <= write_sz);
                        woff += wb;
                        write_sz -= wb;
                } while (write_sz > 0);
        }

        tgs->data_off = 0;
}

static int
tgs_png_render(struct tgs_png* tgs, Lottie_Animation* tgsanim, size_t frame_num)
{
        tgs->frame_num = frame_num;

        lottie_animation_render(tgsanim, frame_num, (uint32_t*)tgs->buffer,
                                width, height, width * 4);

        /* Write PNG image into tgs->data */
        png_structp png;
        png_infop info;
        png = png_create_write_struct(
                PNG_LIBPNG_VER_STRING, NULL, tgs_png_error_fn, NULL);
        if (png == NULL) {
                fprintf(stderr, "png_create_write_struct() failed\n");
                return 1;
        }
        png_set_write_fn(png, tgs, tgs_png_write_fn, NULL);

        info = png_create_info_struct(png);
        if (info == NULL) {
                png_destroy_write_struct(&png, NULL);
                fprintf(stderr, "png_create_info_struct() failed\n");
                return 2;
        }
        png_set_IHDR(png, info, width, height, 8,
                     PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);
        png_set_bgr(png);

        png_set_filter(png, 0, PNG_FAST_FILTERS);
        png_set_compression_level(png, compress_level);

        if (setjmp(png_jmpbuf(png))) {
                png_destroy_write_struct(&png, &info);
                perror("setjmp()");
                return -1;
        }

        png_write_info(png, info);
        png_write_image(png, tgs->rows);
        png_write_end(png, info);

        png_destroy_write_struct(&png, &info);

        return 0;
}

static int
tgs_loop_once(Lottie_Animation* tgsanim)
{
        size_t stop_frame = lottie_animation_get_totalframe(tgsanim);
        double fps = lottie_animation_get_framerate(tgsanim);

        /* Modify stop_frame according to -n parameter */
        if ((nframes > 0) && (start_frame + nframes) < stop_frame)
                stop_frame = start_frame + nframes;

        struct tgs_png tgspng;
        tgs_png_init(&tgspng);

        int frame = start_frame;
        do {
                if (tgs_png_render(&tgspng, tgsanim, frame))
                        goto out;

                if (frame == start_frame)
                        tgs_png_timer_init(&tgspng, frame, fps);
                else
                        tgs_png_timer_sleep(&tgspng, frame);

                tgs_png_flush(&tgspng);
        } while (++frame < stop_frame);

out:
        tgs_png_fini(&tgspng);
        return 0;
}

static int
tgs_animate(char* input)
{
        Lottie_Animation* tgsanim;

        if (!strcmp(input, "-")) {
                /* Read lottie data from stdin */
#define RDSIZE 4096
                size_t dcap = RDSIZE + 1;
                char* data = malloc(dcap);
                assert(data != NULL);
                size_t doff = 0;

                ssize_t rlen;
                while ((rlen = read(0, &data[doff], dcap - 1 - doff)) > 0) {
                        doff += rlen;
                        if (doff >= (dcap - 1)) {
                                dcap += RDSIZE;
                                data = realloc(data, dcap);
                                assert(data != NULL);
                        }
                }
                data[doff] = '\0';
                tgsanim = lottie_animation_from_data(data, "", "");
                free(data);
#undef RDSIZE
        } else {
                tgsanim = lottie_animation_from_file(input);
        }
        if (tgsanim == NULL) {
                fprintf(stderr, "ERR Can't open: %s\n", input);
                return -1;
        }

        if (show_info || debug) {
                size_t frames = lottie_animation_get_totalframe(tgsanim);
                double fps = lottie_animation_get_framerate(tgsanim);
                double dur = lottie_animation_get_duration(tgsanim);
                size_t xw, xh;
                lottie_animation_get_size(tgsanim, &xw, &xh);

                printf("%s: %zux%zu, frames=%zu, fps=%.2f, duration=%.2fs\n",
                       input, xw, xh, frames, fps, dur);

                if (show_info) {
                        return 0;
                        /* NOT REACHED */
                }
        }

        /* Calculate size for the PNG images */
        if ((width == 0) || (height == 0)) {
                size_t xw, xh;
                lottie_animation_get_size(tgsanim, &xw, &xh);
                if ((width == 0) && (height == 0)) {
                        width = round(xw * scale);
                        height = round(xh * scale);
                } else if (width == 0) {
                        assert(height != 0);
                        assert(xh != 0);
                        width = round (1.0 * height * xw / xh);
                } else if (height == 0) {
                        assert(width != 0);
                        assert(xw != 0);
                        height = round(1.0 * width * xh / xw);
                }
        }
        if (debug) {
                printf("PNG size: %dx%d\n", width, height);
        }

        while (loop-- > 0) {
                if (tgs_loop_once(tgsanim))
                        break;
        }

        lottie_animation_destroy(tgsanim);
        return 0;
}

static void
usage(char* prog)
{
        printf("Version %s\n", version);
        printf("usage: %s [-id] [-c L] [-l N] [-z Z] [-o N] [-n N] [-s WxH] INPUT\n",
               prog);
        printf("INPUT could be filename, or -\n");
        printf("\nOptions:\n");
        printf("  -d       Debug run, do not generate PNGs\n");
        printf("  -i       Show info about file.tgs\n");
        printf("  -s WxH   Size of resulting png files\n");
        printf("           W or H can be 0, in this case it is calculated keeping aspect ratio\n");
        printf("  -z Z     Zoom by Z factor, not used if -s is specified\n");
        printf("  -l LOOP  Loop for LOOP times, default=1\n");
        printf("  -o O     Start from O frame\n");
        printf("  -n N     Stop after N frames\n");
        printf("  -c LVL   PNG compress level (default=1)\n");
        printf("  -h       Show version and usage\n");
        exit(0);
}

int
main(int ac, char** av)
{
        int ch;
        while ((ch = getopt(ac, av, "dihc:s:o:n:l:z:")) != -1) {
                switch (ch) {
                case 'c':
                        compress_level = atoi(optarg);
                        break;
                case 'd':
                        debug = true;
                        break;
                case 'i':
                        show_info = true;
                        break;
                case 'l':
                        loop = atoi(optarg);
                        break;
                case 'o':
                        start_frame = atoi(optarg);
                        break;
                case 'n':
                        nframes = atoi(optarg);
                        break;
                case 's':
                        if (2 != sscanf(optarg, "%dx%d", &width, &height)) {
                                fprintf(stderr, "Invalid size specified.\n");
                                usage(av[0]);
                                /* NOT REACHED */
                        }
                        break;
                case 'h':
                default:
                        usage(av[0]);
                        /* NOT REACHED */
                }
        }

        if (optind >= ac) {
                usage(av[0]);
                /* NOT REACHED */
        }

        return tgs_animate(av[optind]);
}
