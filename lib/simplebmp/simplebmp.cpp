/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "simplebmp.h"

#include <algorithm>


#include <cstdio>
int bmp_read_header(BMP* bmp, FILE* fd)
{
    char magic[2];
    fread(magic, 1, 2, fd);
    if (magic[0] != 'B' || magic[1] != 'M')
    {
        return -1;
    }
    fread(&bmp->size, 4, 1, fd);
    fseek(fd, 4, SEEK_CUR); //Seek over the reserved bytes;
    fread(&bmp->pdata, 4, 1, fd);
    fread(&bmp->header_size, 1, 4, fd);
    fread(&bmp->width, 1, 4, fd);
    fread(&bmp->height, 1, 4, fd);
    fread(&bmp->planes, 1, 2, fd);
    fread(&bmp->bits, 1, 2, fd);
    fread(&bmp->compression, 1, 4, fd);
    fread(&bmp->imagesize, 4, 1, fd);
    fread(&bmp->hres, 4, 1, fd);
    fread(&bmp->vres, 4, 1, fd);
    fread(&bmp->colors, 4, 1, fd);
    fread(&bmp->importantcolors, 4, 1, fd);
    if (bmp->compression == BMP_BITFIELDS)
    {
        fread(&bmp->red_mask, 4, 1, fd);
        fread(&bmp->green_mask, 4, 1, fd);
        fread(&bmp->blue_mask, 4, 1, fd);
    }
    return 0;
}

int bmp_write_header(BMP* bmp, FILE* fd)
{
    if (bmp->colors != 0)
    {
        //No support for colour tables.
        return -1;
    }
    if (bmp->compression != BMP_RGB && bmp->compression != BMP_BITFIELDS)
    {
        return -1;
    }

    //Calculate the header size
    size_t header_size = 14; //14 bytes for the bitmap header
    header_size += bmp->header_size;
    // header_size += (4 - (header_size & 3)) & 3; //Pad to 4 byte alignment required by BMP
    printf("Calculated Header Size: %lu\n", header_size);
    bmp->pdata = header_size;
    //Write the bitmap header
    constexpr char magic[2] = {'B', 'M'};
    fwrite(magic, 1, 2, fd);
    fwrite(&bmp->size, 4, 1, fd);
    fseek(fd, 4, SEEK_CUR); //Seek over the reserved bytes;
    fwrite(&bmp->pdata, 4, 1, fd);
    //Write the DIB Header
    fwrite(&bmp->header_size, 1, 4, fd);
    fwrite(&bmp->width, 1, 4, fd);
    fwrite(&bmp->height, 1, 4, fd);
    fwrite(&bmp->planes, 1, 2, fd);
    fwrite(&bmp->bits, 1, 2, fd);
    fwrite(&bmp->compression, 1, 4, fd);
    fwrite(&bmp->imagesize, 4, 1, fd);
    fwrite(&bmp->hres, 4, 1, fd);
    fwrite(&bmp->vres, 4, 1, fd);
    fwrite(&bmp->colors, 4, 1, fd);
    fwrite(&bmp->importantcolors, 4, 1, fd);
    if (bmp->compression == BMP_BITFIELDS)
    {
        fwrite(&bmp->red_mask, 4, 1, fd);
        fwrite(&bmp->green_mask, 4, 1, fd);
        fwrite(&bmp->blue_mask, 4, 1, fd);
    }
    fseek(fd, header_size, SEEK_SET);
    return 0;
}

void bmp_print_header(const BMP* bmp)
{
    printf("Pixel Data Offset: %d\n", bmp->pdata);
    printf("Header Size: %d Bytes\n", bmp->header_size);
    switch (bmp->header_size)
    {
    case 40:
        printf("BITMAPINFOHEADER\n");
        break;
    case 52:
        printf("BITMAPV2INFOHEADER\n");
        break;
    case 56:
        printf("BITMAPV3INFOHEADER\n");
        break;
    case 108:
        printf("BITMAPV4HEADER\n");
        break;
    case 124:
        printf("BITMAPV5HEADER\n");
        break;
    default:
        printf("Unsupported Format\n");
    }
    printf("Width: %d Height: %d\n", bmp->width, bmp->height);
    printf("Bits Per Pixel: %d\n", bmp->bits);
    printf("Compression: %d\n", bmp->compression);
    switch (bmp->compression)
    {
    case BMP_RGB:
        printf("BI_RGB\n");
        break;
    case BMP_RLE8:
        printf("BI_RLE8\n");
        break;
    case BMP_RLE4:
        printf("BI_RLE4\n");
    case BMP_BITFIELDS:
        printf("BI_BITFIELDS\n");
        break;
    case BMP_JPEG:
        printf("BI_JPEG\n");
        break;
    case BMP_PNG:
        printf("BI_PNG\n");
        break;
    case BMP_ALPHABITFIELDS:
        printf("BI_ALPHABITFIELDS\n");
        break;
    case BMP_CMYK:
        printf("BI_CYMK\n");
        break;
    case BMP_CMYKRLE8:
        printf("BI_CMYKRLE8\n");
        break;
    case BMP_CMYKRLE4:
        printf("BI_CMYKRLE4\n");
        break;
    default:
        printf("Unsupported Format\n");
    }
    printf("Image Size: %d Bytes\n", bmp->imagesize);
    printf("Hres %d, Vres: %d\n", bmp->hres, bmp->vres);
    printf("Colors: %d\n", bmp->colors);
    printf("Important Colors: %d\n", bmp->importantcolors);
    if (bmp->compression == BMP_BITFIELDS)
    {
        printf("Red Mask: %x\n", bmp->red_mask);
        printf("Green Mask: %x\n", bmp->green_mask);
        printf("Blue Mask: %x\n", bmp->blue_mask);
    }
}

void bmp_write(BMP* bmp, const uint8_t *output, FILE *fp) {
    if (bmp->bits%8 != 0) return;
    bmp_write_header(bmp, fp);
    const int bytesPerPixel = bmp->bits / 8;
    for (int i = bmp->height-1; i >= 0; i--)
    {
        fwrite(&output[i * bmp->width*bytesPerPixel], bytesPerPixel, bmp->width, fp);
        if((4 - ((bmp->width*bytesPerPixel) & 3)) & 3){
            fseek(fp, (4 - ((bmp->width*bytesPerPixel) & 3)) & 3, SEEK_CUR);
        }
    }
}

void bmp_read_pdata(const BMP* bmp, uint8_t *output, FILE *fp) {
    // TODO: Support other bitmap types
    fseek(fp, bmp->pdata, SEEK_SET);
    if (bmp->compression == BMP_BITFIELDS) {
        const int bytesPerPixel = bmp->bits / 8;
        for (int i = bmp->height-1; i >= 0; i--)
        {
            fread(&output[i * bmp->width*bytesPerPixel], bytesPerPixel, bmp->width, fp);
            if((4 - ((bmp->width*bytesPerPixel) & 3)) & 3){
                fseek(fp, (4 - ((bmp->width*bytesPerPixel) & 3)) & 3, SEEK_CUR);
            }
        }
    }
}