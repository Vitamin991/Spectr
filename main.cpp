#define _CRT_SECURE_NO_WARNINGS 1
#include <fftw3.h>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <GL/glut.h>


#define log(fmt, ...) fprintf(stderr, fmt"\n", ##__VA_ARGS__);
#define ensure(cond, fmt, ...) if (!(cond)) {log(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE);}
#define ret_if(cond, fmt, ...) if (cond) {log(fmt"\n", ##__VA_ARGS__); return -1;}

const char* input_file = "./test_file_student.bin";
int nfft = 16384;
int window_h = 500;
int window_w = 342;
int max_input_count = 5279730;
float freq_disc = 24000;
float freq_l = -500;
float freq_h = 500;
float drawed_value_min = -30;
float drawed_value_max = -0;
int runtime_display = 0;

#define sample_l (freq_l / (freq_disc / nfft))
#define sample_h (freq_h / (freq_disc / nfft))

void drawing(void);
void parse_arg(int argc, char* argv[]);

int main(int argc, char** argv)
{
    parse_arg(argc, argv);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE);
    glutInitWindowSize(window_h, window_w);
    glutCreateWindow("spector");
    glutIdleFunc(drawing);
    drawing();
    drawing();
    glutMainLoop();
    return 0;
}

#define GH 342
#define GW 689968

static int state = 0;
void drawing() {
    static fftwf_complex* in, * out;
    static FILE* input;
    static int input_cnt = 0;
    static int counter = 0;

    switch (state) {
    case 0: // open file
        log("open file");
        state = 1;
        errno = 0;
        input = fopen(input_file, "rb");
        ensure(input, "Failed to open input file: \"%s\", errno: %d (%s)\n",
            input_file, errno, strerror(errno));
        break;
    case 1: // load data
        log("load data");
        state = 2;
        fseek(input, 0L, SEEK_END);
        input_cnt = ftell(input) / (2 * sizeof(float));
        if (input_cnt > max_input_count) {
            input_cnt = max_input_count;
        }
        printf("input_cnt: %d\n", input_cnt);
        fseek(input, 0L, SEEK_SET);

        in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * input_cnt);
        ensure(in, "Failed to alloc input buffer: size: %ld", sizeof(fftwf_complex) * input_cnt);
        ensure(fread((void*)in, sizeof(fftwf_complex), input_cnt, input) == input_cnt,
            "Failed to load data");
        fclose(input);
        gluOrtho2D(0.0, input_cnt - nfft, 0, sample_h - sample_l);
        input = NULL;
        out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * nfft);
        ensure(in, "Failed to alloc output buffer: size: %ld", sizeof(fftwf_complex) * nfft);
        break;
    case 2: // preparing
        log("preparing");
        counter = 0;
        state = 3;
        log("drawing");
        break;
    case 3: case_3: // calculate and draw spector
        // find spector
        {
        fftwf_plan p = fftwf_plan_dft_1d(nfft, &in[counter], out, FFTW_FORWARD, FFTW_ESTIMATE);
        fftwf_execute(p);
        fftwf_destroy_plan(p);
        }

        // draw spector
        glBegin(GL_POINTS);
        for (int q = sample_l; q < sample_h; q++) {
            int i = q % nfft;
            if (i < 0) {
                i += nfft;
            }
            float val = 10 * log10((sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])));
            val = (val - drawed_value_min) / (drawed_value_max - drawed_value_min);
            val = val > 1 ? 1 : val < 0 ? 0 : val;
            glColor3f(val, val, val);
            glVertex2i(counter, q - sample_l);
        }
        glEnd();
        counter++;
        if (counter >= input_cnt - nfft) {
            log("counter: %d, input_cnt: %d, nfft: %d", counter, input_cnt, nfft);
            state = 4;
        }
        if (!runtime_display && state == 3) {
            goto case_3;
        }
        glutSwapBuffers();
        break;
    case 4:
        break;
    }
}

const char* usage = 
"Options:\n"
"    -i --input [path]  input file in compex64 format. Default: ./test_file_student.bin\n"
"    -n --nfft  [N]     FFT order. Default: 16384\n"
"    -h --winh  [pixel] Window hieght in pixel. Default: 500\n"
"    -w --winw  [pixel] Window width in pixel. Default: 342\n"
"    -l --dlen  [len]   Max data len. Default: 5279730\n"
"    -d --dfreq [Hz]    Sampling frequency. Default: 24000 Hz\n"
"       --lfreq [Hz]    bottom frequency to display. Default: -500 Hz\n"
"       --hfreq [Hz]    top frequency to display. Default: 500 Hz\n"
"       --lval  [dB]    bottom value. Default: -30 dB\n"
"       --hval  [dB]    top value. Default: 0 dB\n"
"       --disp          enable runtime display.\n";
int options_parse(int key, char* argument);

void parse_arg(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
#define ARG(key, short, long) if (!strcmp(argv[i], "-" short) || !strcmp(argv[i], "--" long)) { \
    i++; ensure(i < argc, "No arg for --" long); ensure(!options_parse(key, argv[i]), "Failed to parse --" long); } else
        ARG('i', "i", "input")
        ARG('n', "n", "nfft")
        ARG('h', "h", "winh")
        ARG('w', "w", "winw")
        ARG('l', "l", "dlen")
        ARG('d', "d", "dfreq")
#undef ARG
#define ARG(key, long) if (!strcmp(argv[i], "--" long)) { \
    i++; ensure(i < argc, "No arg for --" long); ensure(!options_parse(key, argv[i]), "Failed to parse --" long); } else
        ARG('d', "d", "dfreq")
        ARG(0x100, "lfreq")
        ARG(0x101, "hfreq")
        ARG(0x102, "lval")
        ARG(0x103, "hval")
#undef ARG
        if (!strcmp(argv[i], "--disp")) {
            ensure(!options_parse(0x104, argv[i]), "Failed to parse --disp");
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-?")) {
            log("%s", usage);
            exit(0);
        } else {
            ensure(false, "Unknown arg: \"%s\"", argv[i]);
        }
    }
}

int options_parse(int key, char* argument)
{
    char* tmp;
    switch (key) {
    case 'i':
        input_file = argument;
        break;
#define CASE(key, long, var) \
    case key: var = strtol(argument, &tmp, 10); \
        ret_if(*tmp != 0, "Failed to parse \"%s\" as \"" long "\" arg", argument); \
        break;
            CASE('n', "nfft", nfft)
            CASE('h', "winh", window_h)
            CASE('w', "winw", window_w)
            CASE('l', "dlen", max_input_count)
#undef CASE

#define CASE(key, long, var) \
    case key: var = strtof(argument, &tmp); \
        ret_if(*tmp != 0, "Failed to parse \"%s\" as \"" long "\" arg", argument); \
        break;

            CASE('d',   "dfreq", freq_disc)
            CASE(0x100, "lfreq", freq_l)
            CASE(0x101, "hfreq", freq_h)
            CASE(0x102, "lval", drawed_value_min)
            CASE(0x103, "hval", drawed_value_max)
#undef CASE
    case 0x104:
        runtime_display = 1;
        break;
    default:
        return -1;
    }
    return 0;
}
