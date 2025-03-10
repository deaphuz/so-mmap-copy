#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_THRESHOLD 104857600 // 100 MB

void copyFile(char *srcFilename, char *destFilename, off_t threshold) {
    int srcFile = open(srcFilename, O_RDONLY);
    if (srcFile == -1) {
        perror("Blad otwarcia pliku zrodlowego");
        return;
    }

    struct stat st;
    if (fstat(srcFile, &st) == -1) {
        perror("Blad odczytu informacji o pliku zrodlowym");
        close(srcFile);
        return;
    }

    if (st.st_size <= threshold) {
        // Kopiowanie przy pomocy read/write dla malych plikow
        int destFile = open(destFilename, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
        if (destFile == -1) {
            perror("Blad otwarcia pliku docelowego");
            close(srcFile);
            return;
        }

        char buffer[4096];
        ssize_t bytesRead, bytesWritten;
        while ((bytesRead = read(srcFile, buffer, sizeof(buffer))) > 0) {
            bytesWritten = write(destFile, buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                perror("Blad zapisu do pliku docelowego");
                close(srcFile);
                close(destFile);
                return;
            }
        }

        close(destFile);
	printf("Wykorzystano - read/write\n");
    } else {
        // Kopiowanie przy pomocy mmap/write dla duzych plikow
        int destFile = open(destFilename, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
        if (destFile == -1) {
            perror("Blad otwarcia pliku docelowego");
            close(srcFile);
            return;
        }

        char *srcData = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, srcFile, 0);
        if (srcData == MAP_FAILED) {
            perror("Blad mapowania pliku zrodlowego w pamieci");
            close(srcFile);
            close(destFile);
            return;
        }

        ssize_t bytesWritten = write(destFile, srcData, st.st_size);
        if (bytesWritten != st.st_size) {
            perror("Blad zapisu do pliku docelowego");
	    munmap(srcData, st.st_size);
            close(srcFile);
            close(destFile);
            return;
        }
        munmap(srcData, st.st_size);
        close(destFile);
	printf("Wykorzystano - MMAP/write\n");
    }
    
    close(srcFile);
}


int main() {
    char srcFilename[256];
    char destFilename[256];
    off_t threshold;

    printf("Podaj nazwe pliku zrodlowego: ");
    scanf("%s", srcFilename);

    printf("Podaj nazwe pliku docelowego: ");
    scanf("%s", destFilename);

    printf("Podaj prog rozmiaru (w bajtach), powyzej ktorego plik będzie kopiowany przy pomocy mmap() lub wpisz 0, jezeli chcesz uzyc domyslnych 100 MB: ");
    scanf("%ld", &threshold);
    if(threshold == 0) threshold = DEFAULT_THRESHOLD;

    copyFile(srcFilename, destFilename, threshold);

    printf("Kopiowanie zakonczone.\n");
    return 0;
}
