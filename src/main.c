#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#define MAX_CHAR 256
#define MAX_LEN 32
#define MAX_FILENAME 512

typedef struct node{
    char ch;
    int freq;
    struct node *left, *right, *next;
}node;

typedef struct {
    char filename[MAX_FILENAME];
    long originalSize;
    long compressedSize;
    time_t modifiedTime;
} FileHeader;

char codes[MAX_CHAR][MAX_LEN] = {0};

void printUsage(void){
    printf("Usage: ./compress <command> [options]\n");
    printf("Commands:\n");
    printf("  create <archive> <files...>    - Create compressed archive from files\n");
    printf("  extract <archive> [directory]  - Extract archive to directory\n");
    printf("  list <archive>                 - List contents of archive\n");
}

node* createNode(char ch, int freq){
    node* newNode = (node*)malloc(sizeof(node));
    newNode->ch = ch;
    newNode->freq = freq;
    newNode->left = newNode->right = newNode->next = NULL;
    return newNode;
}

void insertNode(node* newNode, node** head){
    if(*head == NULL || newNode->freq < (*head)->freq){
        newNode->next = *head;
        *head = newNode;
        return;
    }
    
    node* temp = *head;
    while(temp->next != NULL && temp->next->freq <= newNode->freq){
        temp = temp->next;
    }
    newNode->next = temp->next;
    temp->next = newNode;
}

node* ListToNodes(int* frequencyList){
    node* head = NULL;
    for(int i = 0; i < MAX_CHAR; i++){
        if(frequencyList[i] > 0){
            node* newNode = createNode(i, frequencyList[i]);
            insertNode(newNode, &head);
        }
    }
    return head;
}

void LinkTwoMin(node** head){
    if(*head == NULL || (*head)->next == NULL) return;
    
    node* first = *head;
    node* second = first->next;
    
    node* merged = createNode('\0', first->freq + second->freq);
    merged->left = first;
    merged->right = second;
    
    *head = second->next;
    first->next = NULL;
    second->next = NULL;
    
    insertNode(merged, head);
}

node* NodesToTree(node* head){
    while(head != NULL && head->next != NULL){
        LinkTwoMin(&head);
    }
    return head;
}

void freeTree(node* root){
    if(!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

int calculateTreeDepth(node* root) {
    if (!root) return 0;
    if (!root->left && !root->right) return 1;
    
    int leftDepth = calculateTreeDepth(root->left);
    int rightDepth = calculateTreeDepth(root->right);
    return 1 + (leftDepth > rightDepth ? leftDepth : rightDepth);
}

void buildCodes(node* root, char* code, int depth){
    if(!root) return;
    
    if(!root->left && !root->right){
        code[depth] = '\0';
        strcpy(codes[(unsigned char)root->ch], code);
        return;
    }

    code[depth] = '0';
    buildCodes(root->left, code, depth + 1);

    code[depth] = '1';
    buildCodes(root->right, code, depth + 1);
}

int compressFileToArchive(char* inputFile, FILE* output, long* origSize, long* compSize){
    if (!inputFile) {
        printf("Error: inputFile is NULL\n");
        return 0;
    }
    int frequencyList[MAX_CHAR] = {0};
    
    FILE *input = fopen(inputFile, "rb");
    if(!input){
        printf("Error: Cannot open file '%s'\n", inputFile);
        return 0;
    }
    
    // Get file info
    struct stat fileStat;
    stat(inputFile, &fileStat);
    
    // Count character frequencies
    char* fileData = NULL;
    
    // Get file size
    fseek(input, 0, SEEK_END);
    *origSize = ftell(input);
    rewind(input);
    
    if (*origSize == 0) {
        printf("Processing %s...\n", inputFile);
        printf("Warning: File '%s' is empty\n", inputFile);
    
        // Still write header for empty file
        FileHeader header;
        strncpy(header.filename, inputFile, MAX_FILENAME - 1);
        header.filename[MAX_FILENAME - 1] = '\0';
        header.originalSize = 0;
        header.compressedSize = 0;
        header.modifiedTime = fileStat.st_mtime;
    
        fwrite(&header, sizeof(FileHeader), 1, output);
        // Write empty frequency list
        int emptyFreq[MAX_CHAR] = {0};
        fwrite(emptyFreq, sizeof(int), MAX_CHAR, output);
    
        fclose(input);
        return 1; // Return success instead of 0
    }
    
    fileData = malloc(*origSize);
    fread(fileData, 1, *origSize, input);
    
    for (long i = 0; i < *origSize; i++) {
        frequencyList[(unsigned char)fileData[i]]++;
    }
    
    // Clear codes array
    memset(codes, 0, sizeof(codes));
    
    node* head = ListToNodes(frequencyList);
    node* root = NULL;
    
    // Handle single character case
    if(head && head->next == NULL){
        codes[(unsigned char)head->ch][0] = '0';
        codes[(unsigned char)head->ch][1] = '\0';
        root = head;
    } else {
        root = NodesToTree(head);
        char code[MAX_LEN];
        buildCodes(root, code, 0);
    }
    
    // Calculate compressed size
    for (long i = 0; i < *origSize; i++) {
        *compSize += strlen(codes[(unsigned char)fileData[i]]);
    }
    *compSize = (*compSize + 7) / 8;

    // Write file header
    FileHeader header;
    strncpy(header.filename, inputFile, MAX_FILENAME - 1);
    header.filename[MAX_FILENAME - 1] = '\0';
    header.originalSize = *origSize;
    header.compressedSize = *compSize;
    header.modifiedTime = fileStat.st_mtime;
    
    fwrite(&header, sizeof(FileHeader), 1, output);
    fwrite(frequencyList, sizeof(int), MAX_CHAR, output);
    
    // Write compressed data
    unsigned char bitBuffer = 0;
    int bitCount = 0;
    for (long i = 0; i < *origSize; i++) {
        char* code = codes[(unsigned char)fileData[i]];
        for (int j = 0; code[j] != '\0'; j++){
            bitBuffer = (bitBuffer << 1) | (code[j] - '0');
            bitCount++;
            
            if (bitCount == 8){
                fputc(bitBuffer, output);
                bitBuffer = 0;
                bitCount = 0;
            }
        }
    }
    if (bitCount > 0){
        bitBuffer = bitBuffer << (8 - bitCount);
        fputc(bitBuffer,output);
    }
    
    printf("Processing %s...\n", inputFile);
    printf("  Original size: %ld bytes\n", *origSize);
    printf("  Compressed size: %ld bytes\n", *compSize);
    printf("  Compression ratio: %.1f%%\n", ((double)*compSize / *origSize * 100));
    
    free(fileData);
    fclose(input);
    freeTree(root);
    return 1;
}

void createArchive(char* archiveName, char** files, int fileCount){
    printf("=== CREATING ARCHIVE ===\n");
    printf("Archive: %s\n", archiveName);
    
    FILE *archive = fopen(archiveName, "wb");
    if(!archive){
        printf("File Openning Error'%s'\n", archiveName);
        return;
    }
    
    // Write the number of files
    fwrite(&fileCount, sizeof(int), 1, archive);
    
    int successCount = 0;
    long totalOriginal = 0, totalCompressed = 0;
    
    for(int i = 0; i < fileCount; i++){
        long origSize = 0,compSize = 0;
        if(compressFileToArchive(files[i], archive, &origSize, &compSize)){
            successCount++;
            totalOriginal += origSize;
            totalCompressed += compSize;
        }
    }
    
    fclose(archive);
    printf("\nArchive created successfully!\n");
    printf("Total original size: %ld bytes\n", totalOriginal);
    printf("Total compressed size: %ld bytes\n", totalCompressed);
    if (totalOriginal > 0) {
        printf("Overall compression ratio: %.1f%%\n", ((double)totalCompressed / totalOriginal * 100));
    }
}

void listArchive(char* archiveName) {
    printf("=== ARCHIVE CONTENTS ===\n");
    printf("Archive: %s\n", archiveName);
    
    FILE *archive = fopen(archiveName, "rb");
    if(!archive){
        printf("Error: Cannot open archive '%s'\n", archiveName);
        return;
    }
    
    // Get archive creation time
    struct stat archiveStat;
    stat(archiveName, &archiveStat);
    struct tm *timeInfo = localtime(&archiveStat.st_mtime);
    printf("Created: %04d-%02d-%02d %02d:%02d:%02d\n", 
           timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
           timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    
    int fileCount;
    if(fread(&fileCount, sizeof(int), 1, archive) != 1){
        printf("Error: Invalid archive format\n");
        fclose(archive);
        return;
    }
    
    printf("Total files: %d\n\n", fileCount);
    
    for(int i = 0; i < fileCount; i++){
        FileHeader header;
        int frequencyList[MAX_CHAR];
        
        if (fread(&header, sizeof(FileHeader), 1, archive) != 1) break;
        fread(frequencyList, sizeof(int), MAX_CHAR, archive);
        
        // Skip compressed data
        fseek(archive, header.compressedSize, SEEK_CUR);
        
        struct tm *modTime = localtime(&header.modifiedTime);
        printf("File %d: %s\n", i + 1, header.filename);
        printf("  Original size: %ld bytes\n", header.originalSize);
        printf("  Compressed size: %ld bytes\n", header.compressedSize);
        if (header.originalSize > 0) {
            printf("  Compression: %.1f%%\n", ((double)header.compressedSize / header.originalSize) * 100);
        }
        printf("  Modified: %04d-%02d-%02d %02d:%02d:%02d\n", 
               modTime->tm_year + 1900, modTime->tm_mon + 1, modTime->tm_mday,
               modTime->tm_hour, modTime->tm_min, modTime->tm_sec);
        printf("\n");
    }
    
    fclose(archive);
}

node* rebuildTree(int* frequencyList){
    node* head = ListToNodes(frequencyList);
    if(!head) return NULL;
    
    if(head->next == NULL){
        return head;
    } else {
        return NodesToTree(head);
    }
}

void decompressFileFromArchive(FILE* input, const char* outputDir){
    FileHeader header;
    int frequencyList[MAX_CHAR];
    
    // Read file header
    if(fread(&header, sizeof(FileHeader), 1, input) != 1){
        return;
    }
    fread(frequencyList, sizeof(int), MAX_CHAR, input);
    
    // Create output path
    char outputPath[MAX_FILENAME * 2];
    if(outputDir && strlen(outputDir) > 0){
        snprintf(outputPath, sizeof(outputPath), "%s/%s", outputDir, header.filename);
    } else {
        strcpy(outputPath, header.filename);
    }
    
    FILE *output = fopen(outputPath, "wb");
    if(!output){
        printf("Error: Cannot create output file '%s'\n", outputPath);
        return;
    }
    
    // Rebuild Huffman tree
    node* root = rebuildTree(frequencyList);
    
    // Decompress data
    int byte;
    node* current = root;
    long decompressedSize = 0;
    long dataStart = ftell(input);
    
    // Handle Single Character case
    if (root && !root->left && !root->right) {
        for (long i = 0; i < header.originalSize; i++) {
            fputc(root->ch, output);
        }
        fseek(input, dataStart + header.compressedSize, SEEK_SET);
        fclose(output);
        freeTree(root);
        return;
    }

    while(decompressedSize < header.originalSize && (byte = fgetc(input)) != EOF){
        for(int i = 7;i >= 0;i--) {
            int bit = (byte >> i) & 1;
            if(bit == 0) {
                current = current->left;
            }else {
                current = current->right;
            }

            if(!current->left && !current->right){
                fputc(current->ch, output);
                current = root;
                decompressedSize++;
                if (decompressedSize >= header.originalSize) {
                    break; 
                }
            }
        }
    }
    fseek(input, dataStart + header.compressedSize, SEEK_SET);
    fclose(output);
    
    printf("Extracting %s...\n", header.filename);
    printf("  Decompressed: %ld → %ld bytes\n", header.compressedSize, decompressedSize);
    printf("  Saved to: %s\n", outputPath);
    
    freeTree(root);
}

void extractArchive(char* archiveName, const char* outputDir){
    printf("=== EXTRACTING ARCHIVE ===\n");
    printf("Archive: %s\n", archiveName);
    if (outputDir && strlen(outputDir) > 0) {
        printf("Destination: %s\n", outputDir);
    }
    
    FILE *archive = fopen(archiveName, "rb");
    if(!archive){
        printf("Error: Cannot open archive '%s'\n", archiveName);
        return;
    }
    
    int fileCount;
    if(fread(&fileCount, sizeof(int), 1, archive) != 1){
        printf("Error: Invalid archive format\n");
        fclose(archive);
        return;
    }
    
    for(int i = 0; i < fileCount; i++){
        decompressFileFromArchive(archive, outputDir);
    }
    
    fclose(archive);
    printf("\nExtraction complete!\n");
    printf("%d files extracted successfully\n", fileCount);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        printUsage();
        return 1;
    }
    
    if(strcmp(argv[1], "create") == 0){
        if(argc < 4){
            printUsage();
            return 1;
        }
        createArchive(argv[2], &argv[3], argc - 3);
    }
    else if(strcmp(argv[1], "extract") == 0){
        const char* outputDir;//I used char* but there was an error so I write const char*
        if (argc > 3) {
            outputDir = argv[3];
        }
        else {
            outputDir = "";
        }
        extractArchive(argv[2], outputDir);
    }
    else if(strcmp(argv[1], "list") == 0){
        listArchive(argv[2]);
    }
    else {
        printUsage();
        return 1;
    }
    
    return 0;
}
