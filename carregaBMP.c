#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// Define estrutura para o Bitmap
typedef struct {
	int32_t width;
	int32_t height;
	unsigned int rowwidth;
	uint16_t depth;
	unsigned char *data;
}  bitmap_t;

// Le o bitmap de um arquivo
// Retorna o bitmap lido no parametro bitmap
int read_bitmap_from_file(const char* filename, bitmap_t* bitmap) {
	int ret = 0;
	FILE *fd;
	char* buffer;
	char header[54];
	unsigned short offset;
	unsigned char bottomup;
	unsigned int align;
	unsigned int row;
	unsigned int col;

	// Abre o Arquivo
	if ((fd = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Erro abrindo arquivo '%s'.\n", filename);
		ret = -1;
		return ret;
	}

	// Le o cabecalho do bitmap: 54 bytes
	if (fgets(header, 54, fd) == NULL) {
		fprintf(stderr, "Arquivo '%s' nao eh um Bitmap valido (Cabecalho muito curto)\n", filename);
		ret = -2;
		fclose(fd);
		return ret;
	}

	// Checa a assinatura do bitmap para ver se eh um bitmap valido
	if ((header[0] != 0x42) || (header[1] != 0x4D)) {
		fprintf(stderr, "Arquivo '%s' nao eh um Bitmap valido (Assinatura invalida: 0x%X%X)\n", filename, header[0], header[1]);
		ret = -3;
		fclose(fd);
		return ret;
	}

	// Pega o offset para descobrir onde o inicio dos dados da imagem estao
	offset = (header[11] << 8) | header[10];

	// Le a largura do arquivo a partir do cabeçalho
	bitmap->width = *(int32_t*)(header+18);

	// Le a altura do arquivo a partir do cabecalho
	bitmap->height = *(int32_t*)(header+22);

	// Verifica se eh do tipo bottomup
	bottomup = bitmap->height >= 0;

	// Pega as cores da imagem, deve ser 1 para monocromatico
	bitmap->depth = *(uint16_t*)(header+28);

	// Largura de uma linha do byte
	if (bitmap->width % 8) {
		bitmap->rowwidth = ((bitmap->width/8)+1) * bitmap->depth;
	} else {
		bitmap->rowwidth = (bitmap->width/8) * bitmap->depth;
	}

	// Largura de 4-byte por linha, align >= bitmap->rowwidth
	if (bitmap->rowwidth % 4) {
		align = ((bitmap->rowwidth / 4)+1)*4;
	} else {
		align = bitmap->rowwidth;
	}

	fprintf(stdout, "Arquivo '%s' eh um bitmap %ix%ix%i\n", filename, bitmap->width, bitmap->height, bitmap->depth);

	if (bitmap->depth != 1) {
		fprintf(stderr, "Arquivo '%s' nao eh um bitmap monocromatico!\n", filename);
		ret = -4;
		fclose(fd);
		return ret;
	}

	// Pula para o offset para buscar os dados
	fseek(fd, offset, SEEK_SET);

	if ((bitmap->data = (unsigned char*)malloc(align*bitmap->height)) == NULL) {
		fprintf(stderr, "Nao foi possivel alocar memoria para os dados da imagem (%u bytes)!\n", align*bitmap->height);
		ret = -5;
		fclose(fd);
		return ret;
	}

	if ((buffer = (char*)malloc(align)) == NULL) {
		fprintf(stderr, "Nao foi possivel alocar memoria para o buffer de leitura (%u bytes)!\n", align);
		ret = -6;
		free(bitmap->data);
		fclose(fd);
		return ret;
	}

	for (row=0; row<bitmap->height; ++row) {
		fseek(fd, offset+row*align, SEEK_SET);

		// Pega caracter a caracter
		for (col =0; col <= align; ++col) {
			buffer[col] = fgetc(fd);
		}

		if (bottomup) {
			memcpy(bitmap->data+(((bitmap->height-1)-row)*bitmap->rowwidth), buffer, bitmap->rowwidth);
		} else {
			memcpy(bitmap->data+(row*abs(bitmap->width/8)), buffer, bitmap->rowwidth);
		}

	}

	free(buffer);

}

// Imprime X ou espaco em branco
void print_binary(char b, unsigned char length) {
	if (length == 0) length = 8;
	int i;
	for (i=7; i>=8-length; i--) {
		if (b & (1<<i)) {
			printf(" ");
		} else {
			printf("X");
		}
	}
}

// Funcao para imprimir o bitmap
void print_bitmap(bitmap_t* bitmap) {
     unsigned int row;
     unsigned int col;
	for (row=0; row<bitmap->height; ++row) {
		//printf("Linha %3i ",row);
		for (col = 0; col<bitmap->rowwidth; ++col) {
			print_binary(bitmap->data[row*bitmap->rowwidth+col], col == bitmap->rowwidth -1 ? bitmap->width % 8 : 8);
		}
		printf("\n");
	}
}

int main(int argc, char* argv[]) {
	bitmap_t bitmap;

	if (argc != 2) {
		printf("Execute: %s <arquivo>\n",argv[0]);
		printf("  e.g. %s entrada.bmp\n",argv[0]);
		return 1;
	}

	// Le o arquivo
	if (read_bitmap_from_file(argv[1], &bitmap)<0) {
		fprintf(stderr, "Erro abrindo arquivo '%s'!\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// Imprime o bitmap
	print_bitmap(&bitmap);
	// Limpa a memoria usada pelo bitmap
	free(bitmap.data);

	return 0;
}
