#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

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

void mirror() {
    bitMapFile tmp = img;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.r[i][j] = tmp.r[i][cols - j];
            img.g[i][j] = tmp.g[i][cols - j];
            img.b[i][j] = tmp.b[i][cols - j];
        }
    }
}

void glorify() {
    int filter[3][3];
    filter[0][0] = -2;
    filter[0][1] = -1;
    filter[0][2] = 0;
    filter[1][0] = -1;
    filter[1][1] = 1;
    filter[1][2] = 1;
    filter[2][0] = 0;
    filter[2][1] = 1;
    filter[2][2] = 2;
    bitMapFile tmp = img;
    int r = 0, g = 0, b = 0; // This holds the convolution results for an index.
    int x, y; // Used for input matrix index
    for (int i = 0; i < rows - 2; i++) {
        for (int j = 0; j < cols - 2; j++) {
            x = i;
            y = j;
            // Kernel rows and columns are k and l respectively
            for (int k = 0; k < 3; k++) {
                for (int l = 0; l < 3; l++) {
                    r += filter[k][l] * tmp.r[x][y];
                    g += filter[k][l] * tmp.g[x][y];
                    b += filter[k][l] * tmp.b[x][y];
                    y++; // Move right.
                }
                x++; // Move down.
                y = j; // Restart column position
            }
            img.r[i][j] = r > 255 ? 255 : (r < 0 ? 0 : r);
            img.g[i][j] = g > 255 ? 255 : (g < 0 ? 0 : g);
            img.b[i][j] = b > 255 ? 255 : (b < 0 ? 0 : b);
            r = 0;
            g = 0;
            b = 0;
        }
    }
}

void diamond() {
    img.r[floor(rows / 2)][0] = 255;
    img.g[floor(rows / 2)][0] = 255;
    img.b[floor(rows / 2)][0] = 255;

    img.r[0][floor(cols / 2)] = 255;
    img.g[0][floor(cols / 2)] = 255;
    img.b[0][floor(cols / 2)] = 255;

    img.r[floor(rows / 2)][cols - 1] = 255;
    img.g[floor(rows / 2)][cols - 1] = 255;
    img.b[floor(rows / 2)][cols - 1] = 255;

    img.r[rows - 1][floor(cols / 2)] = 255;
    img.g[rows - 1][floor(cols / 2)] = 255;
    img.b[rows - 1][floor(cols / 2)] = 255;

    for (int i = 1; i < floor(rows / 2); i++) {
        for (int j = 0; j < floor(cols / 2); j++) {
            if (i == floor(cols / 2) - j) {
                img.r[i][j] = 255;
                img.g[i][j] = 255;
                img.b[i][j] = 255;
                img.r[i+1][j] = 255;
                img.g[i+1][j] = 255;
                img.b[i+1][j] = 255;
                img.r[i-1][j] = 255;
                img.g[i-1][j] = 255;
                img.b[i-1][j] = 255;
            }
        }
    }

    int y;
    for (int i = floor(rows / 2); i < rows - 1; i++) {
        for (int j = 0; j < floor(cols / 2); j++) {
            y = i - floor(rows / 2);
            if (y == j) {
                img.r[i][j] = 255;
                img.g[i][j] = 255;
                img.b[i][j] = 255;
                img.r[i-1][j] = 255;
                img.g[i-1][j] = 255;
                img.b[i-1][j] = 255;
                img.r[i+1][j] = 255;
                img.g[i+1][j] = 255;
                img.b[i+1][j] = 255;
            }
        }
    }


    for (int i = 1; i < floor(rows / 2); i++) {
        for (int j = floor(cols / 2); j < cols; j++) {
            y = j - floor(cols / 2);
            if (y == i) {
                img.r[i][j] = 255;
                img.g[i][j] = 255;
                img.b[i][j] = 255;
                img.r[i+1][j] = 255;
                img.g[i+1][j] = 255;
                img.b[i+1][j] = 255;
                img.r[i-1][j] = 255;
                img.g[i-1][j] = 255;
                img.b[i-1][j] = 255;
            }
        }
    }

    int x;
    for (int i = floor(rows / 2); i < rows-1; i++) {
        for (int j = floor(cols / 2); j < cols; j++) {
            x = i - floor(rows / 2);
            y = j - floor(cols / 2);
            if (x == floor(cols / 2) - y) {
                img.r[i][j] = 255;
                img.g[i][j] = 255;
                img.b[i][j] = 255;
                img.r[i-1][j] = 255;
                img.g[i-1][j] = 255;
                img.b[i-1][j] = 255;
                img.r[i+1][j] = 255;
                img.g[i+1][j] = 255;
                img.b[i+1][j] = 255;
            }
        }
    }
}

int main(int, char *argv[]) {
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
    mirror();
    glorify();
    diamond();
    // write output file
    writeOutBmp24(fileBuffer, "output.bmp", bufferSize);
    return 0;
}