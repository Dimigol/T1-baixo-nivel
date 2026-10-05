/*

Para compilar de dentro do VS Code:

Windows/Linux: CTRL+SHIFT+B (Terminal -> Run Build Task)
macOS: COMMAND+SHIFT+B

Para executar de dentro do VS Code:

Executar normalmente: CTRL+F5 (Run -> Run Without Debugging)
Debugar: F5 (Run -> Start Debugging)

Pelo terminal:

Windows: mingw32-make
Linux/macOS: make

Para executar:

./bagunceitor [arquivo com a imagem de entrada]

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Para usar strings
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>
#include <stb_image_write.h>

// Um pixel Pixel (24 bits)
typedef struct {
  unsigned char r, g, b;
} Pixel;

// Uma imagem Pixel
typedef struct {
  int width, height;  // largura, altura
  int channels;       // qtd de canais (geralmente 3, RGB)
  Pixel* pixels;
} Img;

// As imagens: original, embaralhada (C+D) e recuperada (D^-1 + C^-1)
Img in, out, rec;

// Protótipos
void load(char* name, Img* pic);
void confusao(Pixel* img, int tam);
void confusao_inversa(Pixel* img, int tam);
void difusao(Pixel* orig, Pixel* dest, int w, int h);
void difusao_inversa(Pixel* orig, Pixel* dest, int w, int h);
int imagens_iguais(Pixel* a, Pixel* b, int tam);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("bagunceitor [origem]\n");
    exit(1);
  }

  // Carrega a imagem original
  load(argv[1], &in);

  // Exibe as dimensões na tela, para conferência
  printf("Origem   : %s %d x %d\n", argv[1], in.width, in.height);

  printf("Processando...\n");

  int tam = in.width * in.height;

  // Cria imagem de saida (embaralhada) e imagem recuperada, do mesmo tamanho
  out = in;
  out.pixels = malloc(tam * sizeof(Pixel));

  rec = in;
  rec.pixels = malloc(tam * sizeof(Pixel));

  //
  // Neste ponto, voce deve implementar os seus algoritmos!
  //
  // IDA: confusao (C) seguida de difusao (D)
  //
  // 1) Copia a original para o buffer de trabalho e aplica a confusao
  //    "no lugar" (cada byte so depende dele mesmo, entao e seguro).
  memcpy(out.pixels, in.pixels, tam * sizeof(Pixel));
  confusao(out.pixels, tam);

  // 2) Aplica a difusao, movendo os pixels confundidos de out para rec
  //    (origem e destino separados, para nao sobrescrever pixels que
  //    ainda nao foram lidos).
  difusao(out.pixels, rec.pixels, in.width, in.height);

  // O resultado da ida (C + D) e o que sai em rec neste ponto; copia de
  // volta para out para gravar "baguncada.png" e liberar rec para a volta.
  memcpy(out.pixels, rec.pixels, tam * sizeof(Pixel));
  stbi_write_png("baguncada.png", out.width, out.height, 3, out.pixels, 0);

  //
  // VOLTA: difusao inversa (D^-1) seguida de confusao inversa (C^-1)
  //
  // 3) Desfaz a difusao, movendo de out (embaralhada) para rec.
  difusao_inversa(out.pixels, rec.pixels, in.width, in.height);

  // 4) Desfaz a confusao "no lugar" em rec.
  confusao_inversa(rec.pixels, tam);

  stbi_write_png("recuperada.png", rec.width, rec.height, 3, rec.pixels, 0);

  //
  // VERIFICACAO: compara a original com a recuperada, byte a byte
  //
  if (imagens_iguais(in.pixels, rec.pixels, tam)) {
    printf("SUCESSO: imagem recuperada e identica a original.\n");
  } else {
    printf("FALHA: imagem recuperada difere da original.\n");
  }

  free(in.pixels);
  free(out.pixels);
  free(rec.pixels);
}

void load(char* name, Img* pic) {
  // Forca 3 canais (RGB) na leitura, para a struct Pixel nao desalinhar
  // caso a imagem de entrada tenha canal alpha (4 canais).
  pic->pixels =
      (Pixel*)stbi_load(name, &pic->width, &pic->height, &pic->channels, 3);
  if (!pic->pixels) {
    printf("Erro de leitura: %s\n", stbi_failure_reason());
    exit(1);
  }
  printf("Load: %d x %d x %d\n", pic->width, pic->height, pic->channels);
  // Exibe um bloco de 8 x 8 pixels em hexadecimal (teste)
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      printf("[%02X %02X %02X] ", pic->pixels[i].r, pic->pixels[i].g,
             pic->pixels[i].b);
    }
    printf("\n");
  }
}

//
// ======================= CONFUSAO (C) =======================
//
// Troca o valor de cada byte por uma funcao dele mesmo (nao move pixels).
// REGRA TEMPORARIA DE TESTE: inverte cada byte de cor (255 - valor), so
// para provar que a estrutura de ida/volta funciona. O grupo deve
// substituir por uma regra autoral (ex.: soma/subtracao ou XOR com uma
// chave, sempre revertida em confusao_inversa).
void confusao(Pixel* img, int tam) {
  unsigned char* p = (unsigned char*)img;
  int n = tam * 3;
  for (int i = 0; i < n; i++) {
    *(p + i) = 255 - *(p + i);
  }
}

// Desfaz exatamente a confusao acima. Como 255 - x e sua propria inversa,
// aplicar a mesma operacao de novo recupera o valor original.
void confusao_inversa(Pixel* img, int tam) {
  unsigned char* p = (unsigned char*)img;
  int n = tam * 3;
  for (int i = 0; i < n; i++) {
    *(p + i) = 255 - *(p + i);
  }
}

//
// ======================= DIFUSAO (D) =======================
//
// Move pixels inteiros de posicao (origem != destino sempre).
// REGRA TEMPORARIA DE TESTE: espelha a imagem (inverte a ordem dos
// pixels no buffer). O grupo deve substituir por uma regra autoral que
// continue sendo uma bijecao (cada posicao de origem vai para uma unica
// posicao de destino, sem sobrar nem repetir nenhuma).
void difusao(Pixel* orig, Pixel* dest, int w, int h) {
  int tam = w * h;
  Pixel* po = orig;
  Pixel* pd = dest + (tam - 1);
  for (int i = 0; i < tam; i++) {
    *pd = *po;
    po++;
    pd--;
  }
}

// Desfaz exatamente a difusao acima. Espelhar duas vezes recupera a
// ordem original dos pixels.
void difusao_inversa(Pixel* orig, Pixel* dest, int w, int h) {
  int tam = w * h;
  Pixel* po = orig;
  Pixel* pd = dest + (tam - 1);
  for (int i = 0; i < tam; i++) {
    *pd = *po;
    po++;
    pd--;
  }
}

//
// ======================= VERIFICACAO =======================
//
// Compara duas imagens byte a byte. Retorna 1 se identicas, 0 se houver
// qualquer diferenca (e mostra em qual byte, para ajudar a debugar).
int imagens_iguais(Pixel* a, Pixel* b, int tam) {
  unsigned char* pa = (unsigned char*)a;
  unsigned char* pb = (unsigned char*)b;
  int n = tam * 3;
  int iguais = 1;
  for (int i = 0; i < n; i++) {
    if (*(pa + i) != *(pb + i)) {
      printf("Diferenca no byte %d: original=%02X recuperada=%02X\n", i,
             *(pa + i), *(pb + i));
      iguais = 0;
    }
  }
  return iguais;
}
