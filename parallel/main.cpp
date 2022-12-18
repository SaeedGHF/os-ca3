#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <chrono>

using namespace std;

#pragma pack(1)

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct bitMap {
    vector <vector<unsigned char >> r;
    vector <vector<unsigned char >> g;
    vector <vector<unsigned char >> b;
} bitMapFile;

bitMapFile img;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize) {
    std::ifstream file(fileName);

    if (file) {
        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);

        buffer = new char[length];
        file.read(&buffer[0], length);

        PBITMAPFILEHEADER file_header;
        PBITMAPINFOHEADER info_header;

        file_header = (PBITMAPFILEHEADER) (&buffer[0]);
        info_header = (PBITMAPINFOHEADER) (&buffer[0] + sizeof(BITMAPFILEHEADER));
        rows = info_header->biHeight;
        cols = info_header->biWidth;
        bufferSize = file_header->bfSize;
        return 1;
    } else {
        cout << "File" << fileName << " doesn't exist!" << endl;
        return 0;
    }
}

void getPixelsFromBMP24(int end, int rows, int cols, char *fileReadBuffer) {
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        img.r.emplace_back(cols, 0);
        img.g.emplace_back(cols, 0);
        img.b.emplace_back(cols, 0);
        for (int j = cols - 1; j >= 0; j--)
            for (int k = 0; k < 3; k++) {
                switch (k) {
                    case 0:
                        // fileReadBuffer[end - count] is the red value
                        img.r[i][j] = fileReadBuffer[end - count++];
                        break;
                    case 1:
                        // fileReadBuffer[end - count] is the green value
                        img.g[i][j] = fileReadBuffer[end - count++];
                        break;
                    case 2:
                        // fileReadBuffer[end - count] is the blue value
                        img.b[i][j] = fileReadBuffer[end - count++];
                        break;
                        // go to the next position in the buffer
                }
            }
    }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize) {
    std::ofstream write(nameOfFileToCreate);
    if (!write) {
        cout << "Failed to write " << nameOfFileToCreate << endl;
        return;
    }
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--)
            for (int k = 0; k < 3; k++) {
                switch (k) {
                    case 0:
                        // write red value in fileBuffer[bufferSize - count]
                        fileBuffer[bufferSize - count++] = img.r[i][j];
                        break;
                    case 1:
                        // write green value in fileBuffer[bufferSize - count]
                        fileBuffer[bufferSize - count++] = img.g[i][j];
                        break;
                    case 2:
                        // write blue value in fileBuffer[bufferSize - count]
                        fileBuffer[bufferSize - count++] = img.b[i][j];
                        break;
                        // go to the next position in the buffer
                }
            }
    }
    write.write(fileBuffer, bufferSize);
}

bitMapFile tmp;

void *mirror_r(void *) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.r[i][j] = tmp.r[i][cols - j];
        }
    }
    return nullptr;
}

void *mirror_g(void *) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.g[i][j] = tmp.g[i][cols - j];
        }
    }
    return nullptr;
}

void *mirror_b(void *) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.b[i][j] = tmp.b[i][cols - j];
        }
    }
    return nullptr;
}

void mirror_old() {
    tmp = img;
    pthread_t *threads = new pthread_t[3];
    pthread_create(&threads[0], nullptr, mirror_r, nullptr);
    pthread_create(&threads[1], nullptr, mirror_g, nullptr);
    pthread_create(&threads[2], nullptr, mirror_b, nullptr);
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], nullptr);
    }
}

void mirror() {
    tmp = img;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.r[i][j] = tmp.r[i][cols - j];
            img.g[i][j] = tmp.g[i][cols - j];
            img.b[i][j] = tmp.b[i][cols - j];
        }
    }
}


int filter[3][3] = {{-2, -1, 0},
                    {-1, 1,  1},
                    {0,  1,  2}};

void *glorify_r(void *) {
    int r = 0;
    int x, y;
    for (int i = 0; i < rows - 2; i++) {
        for (int j = 0; j < cols - 2; j++) {
            x = i;
            y = j;
            // Kernel rows and columns are k and l respectively
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    r += filter[k][l] * tmp.r[x][y];
                    y++; // Move right.
                }
                x++; // Move down.
                y = j; // Restart column position
            }
            img.r[i][j] = r > 255 ? 255 : (r < 0 ? 0 : r);
            r = 0;
        }
    }
    return nullptr;
}

void *glorify_g(void *) {
    int g = 0;
    int x, y;
    for (int i = 0; i < rows - 2; i++) {
        for (int j = 0; j < cols - 2; j++) {
            x = i;
            y = j;
            // Kernel rows and columns are k and l respectively
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    g += filter[k][l] * tmp.g[x][y];
                    y++; // Move right.
                }
                x++; // Move down.
                y = j; // Restart column position
            }
            img.g[i][j] = g > 255 ? 255 : (g < 0 ? 0 : g);
            g = 0;
        }
    }
    return nullptr;
}

void *glorify_b(void *) {
    int b = 0;
    int x, y;
    for (int i = 0; i < rows - 2; i++) {
        for (int j = 0; j < cols - 2; j++) {
            x = i;
            y = j;
            // Kernel rows and columns are k and l respectively
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    b += filter[k][l] * tmp.b[x][y];
                    y++; // Move right.
                }
                x++; // Move down.
                y = j; // Restart column position
            }
            img.b[i][j] = b > 255 ? 255 : (b < 0 ? 0 : b);
            b = 0;
        }
    }
    return nullptr;
}

void glorify() {
    tmp = img;
    pthread_t *threads = new pthread_t[3];
    pthread_create(&threads[0], nullptr, glorify_r, nullptr);
    pthread_create(&threads[1], nullptr, glorify_g, nullptr);
    pthread_create(&threads[2], nullptr, glorify_b, nullptr);
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], nullptr);
    }
}

void white(int x, int y) {
    img.r[x][y] = 255;
    img.g[x][y] = 255;
    img.b[x][y] = 255;
}

void tripleWhite(int x, int y) {
    white(x, y);
    if (x - 1 >= 0)
        white(x - 1, y);
    if (x + 1 < rows)
        white(x + 1, y);
}

void *diamond_up_left(void *) {
    for (int i = 0; i < floor(rows / 2); i++) {
        for (int j = 0; j < floor(cols / 2); j++) {
            if (i == floor(cols / 2) - j) {
                tripleWhite(i, j);
            }
        }
    }
    return nullptr;
}

void *diamond_up_right(void *) {
    for (int i = 0; i < floor(rows / 2); i++) {
        for (int j = floor(cols / 2); j < cols; j++) {
            if (j - floor(cols / 2) == i) {
                tripleWhite(i, j);
            }
        }
    }
    return nullptr;
}

void *diamond_down_left(void *) {
    for (int i = floor(rows / 2); i < rows; i++) {
        for (int j = 0; j < floor(cols / 2); j++) {
            if (i - floor(rows / 2) == j) {
                tripleWhite(i, j);
            }
        }
    }
    return nullptr;
}

void *diamond_down_right(void *) {
    for (int i = floor(rows / 2); i < rows; i++) {
        for (int j = floor(cols / 2); j < cols; j++) {
            if (i - floor(rows / 2) == cols - j) {
                tripleWhite(i, j);
            }
        }
    }
    return nullptr;
}

void diamond() {
    tmp = img;
    pthread_t *threads = new pthread_t[4];
    pthread_create(&threads[0], nullptr, diamond_up_left, nullptr);
    pthread_create(&threads[1], nullptr, diamond_up_right, nullptr);
    pthread_create(&threads[2], nullptr, diamond_down_left, nullptr);
    pthread_create(&threads[3], nullptr, diamond_down_right, nullptr);
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], nullptr);
    }
}

int main(int, char *argv[]) {
    auto start = chrono::high_resolution_clock::now();
    char *fileBuffer;
    int bufferSize;
    char *fileName = argv[1];
    if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize)) {
        cout << "File read error" << endl;
        return 1;
    }
    // read input file
    getPixelsFromBMP24(bufferSize, rows, cols, fileBuffer);
    // apply filters
    tmp = img;
    pthread_t *threads = new pthread_t[10];
    pthread_create(&threads[0], nullptr, mirror_r, nullptr);
    pthread_create(&threads[1], nullptr, mirror_g, nullptr);
    pthread_create(&threads[2], nullptr, mirror_b, nullptr);
    pthread_create(&threads[3], nullptr, glorify_r, nullptr);
    pthread_create(&threads[4], nullptr, glorify_g, nullptr);
    pthread_create(&threads[5], nullptr, glorify_b, nullptr);
    pthread_create(&threads[6], nullptr, diamond_up_left, nullptr);
    pthread_create(&threads[7], nullptr, diamond_up_right, nullptr);
    pthread_create(&threads[8], nullptr, diamond_down_left, nullptr);
    pthread_create(&threads[9], nullptr, diamond_down_right, nullptr);
    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], nullptr);
    }
    // write output file
    writeOutBmp24(fileBuffer, "output.bmp", bufferSize);
    // execution time
    auto end = chrono::high_resolution_clock::now();
    cout << "Execution Time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << endl;
    return 0;
}