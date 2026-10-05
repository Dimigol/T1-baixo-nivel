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
void confusao(Pixel* img, size_t tam);
void confusao_inversa(Pixel* img, size_t tam);
void difusao(Pixel* orig, Pixel* dest, int w, int h);
void difusao_inversa(Pixel* orig, Pixel* dest, int w, int h);
int imagens_iguais(Pixel* a, Pixel* b, size_t tam);

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

  size_t tam = (size_t)in.width * (size_t)in.height;

  // Cria imagem de saida (embaralhada) e imagem recuperada, do mesmo tamanho
  out = in;
  out.pixels = malloc(tam * sizeof(Pixel));

  rec = in;
  rec.pixels = malloc(tam * sizeof(Pixel));

  if (!out.pixels || !rec.pixels) {
    printf("Erro: memoria insuficiente\n");
    exit(1);
  }

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
  //    ainda nao foram lidos). rec passa a ser a imagem baguncada.
  difusao(out.pixels, rec.pixels, in.width, in.height);

  int ok = 1;
  if (!stbi_write_png("baguncada.png", rec.width, rec.height, 3, rec.pixels,
                      0)) {
    printf("Erro: nao foi possivel gravar baguncada.png\n");
    ok = 0;
  }

  //
  // VOLTA: difusao inversa (D^-1) seguida de confusao inversa (C^-1)
  //
  // 3) Desfaz a difusao, movendo de rec (baguncada) para out.
  difusao_inversa(rec.pixels, out.pixels, in.width, in.height);

  // 4) Desfaz a confusao "no lugar" em out, que passa a ser a recuperada.
  confusao_inversa(out.pixels, tam);

  if (!stbi_write_png("recuperada.png", out.width, out.height, 3, out.pixels,
                      0)) {
    printf("Erro: nao foi possivel gravar recuperada.png\n");
    ok = 0;
  }

  //
  // VERIFICACAO: compara a original com a recuperada, byte a byte
  //
  if (!imagens_iguais(in.pixels, out.pixels, tam)) {
    printf("FALHA: imagem recuperada difere da original.\n");
    ok = 0;
  } else if (ok) {
    printf("SUCESSO: imagem recuperada e identica a original.\n");
  }

  free(in.pixels);
  free(out.pixels);
  free(rec.pixels);
  return ok ? 0 : 1;
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
  // stbi_load devolve em channels o numero de canais do arquivo; os dados
  // em memoria sao sempre RGB (3 canais).
  pic->channels = 3;
  printf("Load: %d x %d x %d\n", pic->width, pic->height, pic->channels);
  // Exibe um bloco de 8 x 8 pixels em hexadecimal (teste)
  int lin = pic->height < 8 ? pic->height : 8;
  int col = pic->width < 8 ? pic->width : 8;
  for (int i = 0; i < lin; i++) {
    for (int j = 0; j < col; j++) {
      Pixel* px = pic->pixels + (size_t)i * pic->width + j;
      printf("[%02X %02X %02X] ", px->r, px->g, px->b);
    }
    printf("\n");
  }
}

//
// ======================= CONFUSAO (C) =======================
//
// Troca o valor de cada byte por uma funcao dele mesmo (nao move pixels).
// REGRA TEMPORARIA DE TESTE: soma uma chave fixa a cada byte (mod 256, pois
// e unsigned char). O grupo deve substituir por uma regra autoral. A
// inversa e uma regra DIFERENTE (subtracao), para o teste de ida/volta
// realmente exercitar a funcao inversa.
#define CHAVE_CONFUSAO 91

void confusao(Pixel* img, size_t tam) {
  unsigned char* p = (unsigned char*)img;
  size_t n = tam * 3;
  for (size_t i = 0; i < n; i++) {
    *(p + i) = (unsigned char)(*(p + i) + CHAVE_CONFUSAO);
  }
}

// Desfaz exatamente a confusao acima: subtrai a mesma chave (mod 256).
void confusao_inversa(Pixel* img, size_t tam) {
  unsigned char* p = (unsigned char*)img;
  size_t n = tam * 3;
  for (size_t i = 0; i < n; i++) {
    *(p + i) = (unsigned char)(*(p + i) - CHAVE_CONFUSAO);
  }
}

//
// ======================= DIFUSAO (D) =======================
//
// Move pixels inteiros de posicao.
// REGRA TEMPORARIA DE TESTE: deslocamento circular de DESLOC_DIFUSAO
// pixels no buffer (o pixel i vai para (i + desloc) % tam). O grupo deve
// substituir por uma regra autoral que continue sendo uma bijecao (cada
// posicao de origem vai para uma unica posicao de destino, sem sobrar nem
// repetir nenhuma).
#define DESLOC_DIFUSAO 100003

void difusao(Pixel* orig, Pixel* dest, int w, int h) {
  size_t tam = (size_t)w * (size_t)h;
  size_t desloc = DESLOC_DIFUSAO % tam;
  Pixel* po = orig;
  for (size_t i = 0; i < tam; i++) {
    *(dest + (i + desloc) % tam) = *po;
    po++;
  }
}

// Desfaz exatamente a difusao acima: traz de volta o pixel que foi
// deslocado, lendo da posicao (i + desloc) % tam.
void difusao_inversa(Pixel* orig, Pixel* dest, int w, int h) {
  size_t tam = (size_t)w * (size_t)h;
  size_t desloc = DESLOC_DIFUSAO % tam;
  Pixel* pd = dest;
  for (size_t i = 0; i < tam; i++) {
    *pd = *(orig + (i + desloc) % tam);
    pd++;
  }
}

//
// ======================= VERIFICACAO =======================
//
// Compara duas imagens byte a byte. Retorna 1 se identicas, 0 se houver
// qualquer diferenca. Mostra so as primeiras diferencas (para ajudar a
// debugar sem inundar o terminal) e o total no final.
#define MAX_DIFS_MOSTRADAS 10

int imagens_iguais(Pixel* a, Pixel* b, size_t tam) {
  unsigned char* pa = (unsigned char*)a;
  unsigned char* pb = (unsigned char*)b;
  size_t n = tam * 3;
  size_t difs = 0;
  for (size_t i = 0; i < n; i++) {
    if (*(pa + i) != *(pb + i)) {
      if (difs < MAX_DIFS_MOSTRADAS) {
        printf("Diferenca no byte %zu: original=%02X recuperada=%02X\n", i,
               *(pa + i), *(pb + i));
      }
      difs++;
    }
  }
  if (difs > MAX_DIFS_MOSTRADAS) {
    printf("... e mais %zu bytes diferentes (total: %zu)\n",
           difs - MAX_DIFS_MOSTRADAS, difs);
  }
  return difs == 0;
}
