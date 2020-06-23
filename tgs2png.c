#ifndef _WIN32
#include <libgen.h>
#else
#include <windows.h>
#endif  /* _WIN32 */

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#include <rlottie_capi.h>
#include <png.h>
#include <zlib.h>

const char* version = "0.0.1";

static bool show_info = false;
static bool uncompress_input = false;
static int start_frame = 0;
static int nframes = -1;
static float scale = 1.0;
static int loop = 1;
static int width = 0;
static int height = 0;
static float speed = 1.0;

struct tgs_png {
        png_structp png = NULL;
        png_infop info = NULL;
        png_bytep rows = NULL;

        /*
         * timeval of last flush
         * Used to calculate sleeping time
         */
        struct timeval flush_tv;
};

/*
 * Render FRAME_NUM frame into png buffer
 */
static int
tgs_render_frame(const Lottie_Animation* tgsanim, size_t frame_num)
{
        png_set_write_fn();
}

void
tgs_png_init(struct tgs_png* png)
{
        memset(png, '\0', sizeof(struct tgs_png));
        png->png = NULL;
        png->info = NULL;

}

void
tgs_png_fini(struct tgs_png* png)
{
        if (png->info != NULL)
                png_free_data(png->png, png->info, PNG_FREE_ALL, -1);
        if (png->png != NULL)
                png_destroy_write_struct(&png->png, (png_infopp)NULL);
        if (png->rows != NULL)
                free(png->rows);
}

void
tgs_png_flush(struct tgs_png* png)
{
}

static int
tgs_png_render(struct tgs_png* png, const Lottie_Animation* tgsanim,
               size_t frame_num)
{
        return 0;
}

static int
tgs_loop_once(const Lottie_Animation* tgsanim)
{
        size_t stop_frame = lottie_animation_get_totalframe(tgsanim);
        double fps = lottie_animation_get_framerate(tgsanim);

        /* Modify stop_frame according to -n parameter */
        if ((nframes > 0) && (start_frame + nframes) < stop_frame)
                stop_frame = start_frame + nframes;

        struct tgs_png tgspng;
        tgs_png_init(&tgspng);

        if (setjmp(png_jmpbuf(tgspng.png))) {
                goto out;
        }

        int frame = start_frame;
        tgs_png_render(&tgspng, tgsanim, frame);
        do {
                tgs_png_flush(&tgspng);

                tgs_png_render(&tgspng, tgsanim, ++frame);
                /* TODO: sleep */
        } while (frame < frames_count);

out:
        tgs_png_fini(&tgspng);
        return 0;
}

static int
tgs_animate(char* input)
{
        Lottie_Animation* tgsanim;

        tgsanim = lottie_animation_from_file(input);
        if (tgsanim == NULL) {
                fprintf(stderr, "ERR Can't open: %s\n", input);
                fprintf(stderr, "Forgot to specify -u for gzipped tgs?\n");
                return -1;
        }

        if (show_info) {
                size_t frames = lottie_animation_get_totalframe(tgsanim);
                double fps = lottie_animation_get_framerate(tgsanim);
                double dur = lottie_animation_get_duration(tgsanim);
                size_t xw, xh;
                lottie_animation_get_size(tgsanim, &xw, &xh);

                printf("%s: %zux%zu, frames=%zu, fps=%.2f, duration=%.2fs\n",
                       input, xw, xh, fps, frames, dur);
                return 0;
                /* NOT REACHED */
        }

        if ((width == 0) && (height == 0)) {
                size_t xw, xh;
                lottie_animation_get_size(tgsanim, &xw, &xh);
                if (scale != 1.0) {
                        width = round(xw * scale);
                        height = round(xh * scale);
                }
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
        printf("usage: %s [-ui] [-l N] [-z Z] [-o N] [-n N] [-s WxH] <file.tgs>\n",
               prog);
        printf("\t-i       Show info about file.tgs\n");
        printf("\t-s WxH   Size of resulting png files\n");
        printf("\n-z Z     Zoom by Z factor, not used if -s is specified\n");
        printf("\t-u       Uncompress input\n");
        printf("\t-l LOOP  Loop for LOOP times, default=1\n");
        printf("\t-o O     Start from O frame\n");
        printf("\t-n N     Stop after N frames\n");
        printf("\t-h       Show version and usage\n");
        exit(0);
}

int
main(int ac, char** av)
{
        int ch;
        while ((ch = getopt(ac, av, "ihus:o:n:l:z:")) != -1) {
                switch (ch) {
                case 'i':
                        show_info = true;
                        break;
                case 'u':
                        uncompress_input = true;
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
