/*Universidade de Brasilia
Instituto de Ciencias Exatas
Departamento de Ciencia da Computacao
Algoritmos e Programacao de Computadores – 2/2017
Aluno: Guilherme Mendel de Almeida Nascimento
Matricula: 17/0143970
Turma: A
Versao do compilador: <versao utilizada>
Descricao: Jogo semelhante ao pac-man, em que se coleta pontos enquanto foge do inimigo X*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
	#define CLEAR system("cls")
#else
	#define CLEAR system("clear")
#endif

// cores no printf
	#define BOLD "\x1b[1m"
	#define CROSSEDOUT "\x1b[9m"
	#define ITALIC "\x1b[3m"
	#define REVERSE "\x1b[7m"
	#define BLACK "\x1B[30m"
	#define RED "\x1B[31m"
	#define GREEN "\x1B[32m"
	#define YELLOW "\x1B[33m"
	#define BLUE "\x1B[34m"
	#define MAGENTA "\x1B[35m"
	#define CIAN "\x1B[36m"
	#define NORMAL "\x1B[0m"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// configuracoes

	// config de debug
		int debug=0;
		int sleep=0;
		#define DEBUG_XY (debug? printf("\n %d %d\n", y, x):sleep++)
		#define DEBUG_RESET (debug? (printf( BOLD RED "\n RESETED UNIT\n" NORMAL ), getchar()):sleep++)
		#define DEBUG_MOVE (debug? (printf( BOLD RED "\n MOVED UNIT\n" NORMAL ), getchar()):sleep++)
		#define DEBUG_D (debug? (printf("\n dt.dy: %d\n dt.dx: %d\n dt.daby: %d\n dt.dabx: %d\n", dt.dy, dt.dx, dt.daby, dt.dabx), getchar()):sleep++);
		#define DEBUG_COLLECT (debug? printf( BOLD RED "\n O COLLECTED\n" NORMAL ), getchar():sleep++)
		#define DEBUG_QT (debug? printf(" X: %d\n B: %d\n O: %d\n", qtX, qtB, qtO):sleep++)

	// determina atributos do jogo
		int running=1; /* o aplicativo roda */
		int playing; /* a partida roda */
		int ranked=0; /* a partida é rankeada */
		int turn,
		 score,
		  energy,
		   difficulty=-1/* -1: custom, 0: muito facil, 1: facil, 2: medio, 3: dificil, 4: muito dificil */;
		typedef struct{
					char name[101];
					int hscore;		
				} Rank;/* vai armazenar valores a serem comparados no modo raqueado */
		#define moves (energy-turn+1)
		#define mag(n) (n<0?-n:n) /* retorna a magnitude de um numero */
		#define rt_lowest_mag(n0,n1) (mag(n0)<=mag(n1)?n0:n1) /* retorna a menor magnitude entre argmts */

	// determina os fins de jogo
		int cornered; /* encurralado */
		int murdered; /* assassiado */
		int starved; /* morto de fome (fim de jogadas) */
		int blownup; /* morto por uma bomba */

	// config do tabuleiro
		#define HGT 101
		#define WDT 101
		int hgt /* altura{height} */;
		int wdt /* largura{width} */;
		char board[HGT][WDT];

	// config do C
		int x, y; /* coordenadas do C */

	// config do B
		int bc[101][101][2]; /* bc[identifica o B][referencia uma instancia desse B][coordenada: 0=y, 1=x] */
		int qtB/* quantidade{quantity} B */,
		 trB/* rastro{trail} (limite de instancias) B */;
		#define by (bc[i][j][0]) /* guardam as coordenadas de um B(i,j) para manter o cod limpo */
		#define bx (bc[i][j][1])

	// config do X
		int xc[151][2]/* xc[identifica o X][coordenada: 0=y, 1=x] */,
		 ctX[151]/* contador{counter} de cada X - quando positivo, o X e pacifico, quando negativo, o X persegue o C */,
		  cdX[151]/* tempo de recarga{cooldown} do X, apos morto */;
		int qtX/* quantidade{quantity} X */,
		 bhX/* comportamento{behavior} X - define por quantos turnos ele e passivo */,
		  tcdX/* tempo minimo de recarga de um X morto */,
		   X_eats_diag/* determina se o X come diagonalmente (como um peao) */,
		    X_warmonger/* se bhX for 0, ele age sempre agressivamente */,
		     X_improved_percep/* torna o X mais eficaz em contornar obstaculos e perseguir o C */,
		      X_cross_screen/* determina se o X calcula a trajetoria atraves da tela */;
		#define X_better_cross ((X_cross_screen)&&(X_improved_percep))
		#define xy (xc[i][0]) /* guardam as coordenadas de um X(i) para manter o cod limpo */
		#define xx (xc[i][1])

	// config do O
		int oc[201][2]/* oc[identifica o O][coordenada: 0=y, 1=x] */,
		 ctO[201]/* contador de cada O, utilizado para determinar quando eles reaparecerao apos serem coletados */;
		int qtO/* quantidade de O's */;
		#define oy (oc[i][0])
		#define ox (oc[i][1])
		#define O_value ((((sqrt(hgt*wdt)/30)*(100/(qtO/2))*(mag(mag(difficulty)-1))/2+0.02*qtO-(15/sqrt(hgt*wdt))*4)*qtX)/5-1)
		#define O_energy (((sqrt(hgt*wdt)/30)*(10000/((qtO/2)*energy)+1)-mag(difficulty)/2)+0.01*qtO+0.002*energy)

	// config do Q
		int qc[101][2]/* qc[identifica o Q][coordenada: 0=y, 1=x] */,
		 ctQ[101]/* contador de cada bomba */;
		int qtQ=1/* quantidade maxima de bombas */,
		 cdQ=7/* tempo de recarga{cooldown} medio de cada bomba */,
		  rdQ=6/* alcance{radius} da bomba */, 
		   tQ_base=6/*valor medio que a bomba levara para explodir (turnos)*/, 
		    tQ_range /* variacao da medida de tempo que a bomba levara para explodir (turnos)*/;
		#define qy (qc[i][0])
		#define qx (qc[i][1])
		#define ger_timer (((tQ_base)-(tQ_range)+(rand()%(2*(tQ_range)+1)))*-1)
		#define ger_cd (rand()%(cdQ)+cdQ/2)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macros de logica complexa

	// checam se um char(ch) especifico se encontra numa direcao
		#define char_above(ch, cy, cx) (((cy-1==-1)&&(board[hgt-1][cx]==ch)||(board[cy-1][cx]==ch)))
		#define char_left(ch, cy, cx) (((cx-1==-1)&&(board[cy][wdt-1]==ch)||(board[cy][cx-1]==ch)))
		#define char_below(ch, cy, cx) (((cy+1==hgt)&&(board[0][cx]==ch))||(board[cy+1][cx]==ch))
		#define char_right(ch, cy, cx) (((cx+1==wdt)&&(board[cy][0]==ch))||(board[cy][cx+1]==ch))

	// checam se o espaco esta disponivel
		#define empty_above(cy, cx) (char_above('.', cy, cx)||(char_above('#', cy, cx)))
		#define empty_left(cy, cx) (char_left('.', cy, cx)||(char_left('#', cy, cx)))
		#define empty_below(cy, cx) (char_below('.', cy, cx)||(char_below('#', cy, cx)))
		#define empty_right(cy, cx) (char_right('.', cy, cx)||(char_right('#', cy, cx)))

	// encurralado
		#define kinky(cy, cx) ((!empty_above(cy,cx))&&(!empty_left(cy,cx))&&(!empty_below(cy,cx))&&(!empty_right(cy,cx)))

            
	// checa se ha um char(ch) especifico em torno de uma coordenada
		#define char_around(ch, cy, cx) (char_above(ch,cy,cx)||char_left(ch,cy,cx)||char_below(ch,cy,cx)||char_right(ch,cy,cx)||char_above(ch,cy,(cx-1))||char_above(ch,cy,(cx+1))||char_below(ch,cy,(cx-1))||char_below(ch,cy,(cx+1)))

	// checa se o C esta ao alcance de um X(i)
		#define eat_range(dt) (((dt.dy==0)&&(mag(dt.dx)==1))||((dt.dx==0)&&(mag(dt.dy)==1))||((dt.dy==0)&&(mag(dt.dabx)==1))||((dt.dx==0)&&(mag(dt.daby)==1))||X_eats_diag&&(((mag(dt.dy)==1)&&(mag(dt.dx))==1)||((mag(dt.dy)==1)&&(mag(dt.dabx))==1)||((mag(dt.daby)==1)&&(mag(dt.dx)==1))||((mag(dt.daby)==1)&&(mag(dt.dabx)==1))))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// declaracao de funcoes e metodos

	// funcoes de debug
		void debug_set_X(int i, int cy, int cx);
		void debug_set_B(int i, int j, int cy, int cx);
		void debug_set_Z(int cy, int cx);

	// funcoes de menu
		void logo(); /* imprime o título do jogo na tela */
		void menu(); /* menun principal */
		void play(); /* inicia uma partida */
		void config(); /* permite a configuracao dos personagens */
		void show_ranking(); /* mostra o ranking */
		void instruct(); /* apresenta as instrucoes */
		void game_over(); /* imprime game over na tela e a causa da morte */
		void config_board(); /* config tabuleiro */
		void config_npcs(); /* configurar os personages */
		void toggle_ranked(); /* habilitar o modo ranqueado */
		void show_ranking(); /* mostra os rankings */
		void update_ranking(Rank player); /* atualiza os rankings */

	// funcoes de tabuleiro
		void init_board(); /* inicializa o tabuleiro */
		void update_board(); /* atualiza o tabuleiro com as novas coordenadas */
		void render_board(); /* renderiza o tabuleiro na tela */

	// funcoes aritmeticas
		int lowest4_uns(int n0, int n1, int n2, int n3); /* acha o maior modulo entre os 4 numeros dados */

	// metodos gerais
		void mov(int dr, int* cy, int* cx); /* move uma unidade (y,x) qualquer no sentido dr */
		void warp_b(int* cy, int* cx);
		#define rand_mov(cy, cx) (mov(rand()%(4), cy, cx)) /* move uma unidade (y,x) qualquer em um sentido aleatorio */
		void reset(int* cy, int* cx); /* gera novas coordenadas para uma unidade (y,x) qualquer */
		int find_ch(char ch, int cy, int cx); /* retorna o i correspondente do char nessa coordenada */
		void load(int* ad); /* carrega um valor do arquivo config.txt ao seu endereco correspondente */
		void loadall(); /* carrega todas as configuracoes do arquivo txt */
		void store(int* ad); /* atualiza um valor do arquivo.txt */

	// metodos do C
		void input(); /* recebe os comandos */

	// metodos do B
		void init_B(); /* posiciona os B's */
		void mov_B(); /* move os B's */

	// metodos do X
		void init_X(); /* posiciona os X's */
		void mov_X(); /* move os X's */
		void chase_C(int i); /* faz um X(i) perseguir o C */
		typedef struct { /* atua na comparacao de distancias */
			int dy; /* distancia eixo y */
			int dx; /* distancia eixo x */
			int daby; /* distancia atraves da borda{distance across bank} eixo y */
			int dabx; /* distancia atraves da borda eixo x */
		} dist;
		dist calc_d(int i); /* calcula os atributos de uma variavel dist com relacao a um X(i) */
		void mov_s(int dr, int i, dist dt); /* gera um movimento com base nas config de X que pode contornar obstaculos */
		void outline(int idr/*intended direction*/, int i, dist dt/*auxiliary distance*/, int alt, int n);
		/* o metodo outline recebe um referenciador i (como os outros metodos), uma direcao bloqueada idr, que servira de parametro nos calculos, uma distancia entre o C e o X perpendicular a idr e uma variavel alt que serve como modificador do sinal de adt. Depos, calcula o sentido que o X deve seguir para contornar obstaculos, com uma eficacia ate razoavel. n atua para evitar um possivel stack overflow */

	// metodos do O
		void init_O(); /* posiciona os O's */
		void check_O(); /* faz o C coletar um O */


	// metodos do Q
		void init_Q(); /* inicia os contadores das bombas */
		void update_Q(); /* atualiza as situacoes das bombas */
		void blast_Q(int i); /* explode a bomba Q(i) */
		void clear_Q(); /* remove os chars da explosao */


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// programa

int main () {
	FILE *fp;
	fp=fopen("config.txt", "r");
	if(!fp)
	{
		fp=fopen("config.txt", "w");
		fprintf(fp, "020 020\n007\n007\n0\n1\n1\n010\n010\n002\n010\n06\n015\n010");
	}
	fclose(fp);
	loadall();

	printf("\ncarregando...\n");
	CLEAR;	
	logo();
	while (running)	menu();
	printf("\n Ate a proxima!\n");
	getchar();
}

void load(int* ad) {
	FILE *fp;
	fp=fopen("config.txt", "r");

	if((ad)==(&wdt))
	{
		fscanf(fp, "%d", &wdt);
	}
	if((ad)==(&hgt))
	{
		fseek(fp, 4, SEEK_SET);
		fscanf(fp, "%d", &hgt);
	}
	if((ad)==(&qtX))
	{
		fseek(fp, 9, SEEK_SET);
		fscanf(fp, "%d", &qtX);
	}
	if((ad)==(&bhX))
	{
		fseek(fp, 14, SEEK_SET);
		fscanf(fp, "%d", &bhX);
	}
	if((ad)==(&X_eats_diag))
	{
		fseek(fp, 19, SEEK_SET);
		fscanf(fp, "%d", &X_eats_diag);
	}
	if((ad)==(&X_improved_percep))
	{
		fseek(fp, 22, SEEK_SET);
		fscanf(fp, "%d", &X_improved_percep);
	}
	if((ad)==(&X_cross_screen))
	{
		fseek(fp, 25, SEEK_SET);
		fscanf(fp, "%d", &X_cross_screen);
	}
	if((ad)==(&qtB))
	{
		fseek(fp, 28, SEEK_SET);
		fscanf(fp, "%d", &qtB);
	}
	if((ad)==(&trB))
	{
		fseek(fp, 33, SEEK_SET);
		fscanf(fp, "%d", &trB);
	}
	if((ad)==(&qtQ))
	{
		fseek(fp, 38, SEEK_SET);
		fscanf(fp, "%d", &qtQ);
	}
	if((ad)==(&cdQ))
	{
		fseek(fp, 43, SEEK_SET);
		fscanf(fp, "%d", &cdQ);
	}
	if((ad)==(&tQ_base))
	{
		fseek(fp, 48, SEEK_SET);
		fscanf(fp, "%d", &tQ_base);
	}
	if((ad)==(&rdQ))
	{
		fseek(fp, 52, SEEK_SET);
		fscanf(fp, "%d", &rdQ);
	}
	if((ad)==(&qtO))
	{
		fseek(fp, 57, SEEK_SET);
		fscanf(fp, "%d", &qtO);
	}

	fclose(fp);
}

void loadall() {
	FILE *fp;
	fp=fopen("config.txt", "r");
	fscanf(fp, "%d", &wdt);
	fscanf(fp, "%d", &hgt);
	fscanf(fp, "%d", &qtX);
	fscanf(fp, "%d", &bhX);
	fscanf(fp, "%d", &X_eats_diag);
	fscanf(fp, "%d", &X_improved_percep);
	fscanf(fp, "%d", &X_cross_screen);
	fscanf(fp, "%d", &qtB);
	fscanf(fp, "%d", &trB);
	fscanf(fp, "%d", &qtQ);
	fscanf(fp, "%d", &cdQ);
	fscanf(fp, "%d", &tQ_base);
	fscanf(fp, "%d", &rdQ);
	fscanf(fp, "%d", &qtO);
	fclose(fp);
}

void store(int* ad) {
	FILE *fp;
	fp=fopen("config.txt", "r+");

	if((ad)==(&wdt))
	{
		if(wdt>99) fprintf(fp, "%d", wdt);
		else if(wdt>9) fprintf(fp, "0%d", wdt);
		else fprintf(fp, "00%d", wdt);
	}
	if((ad)==(&hgt))
	{
		fseek(fp, 4, SEEK_SET);
		if(hgt>99) fprintf(fp, "%d", hgt);
		else if(hgt>9) fprintf(fp, "0%d", hgt);
		else fprintf(fp, "00%d", hgt);
	}
	if((ad)==(&qtX))
	{
		fseek(fp, 9, SEEK_SET);
		if(qtX>99) fprintf(fp, "%d", qtX);
		else if(qtX>9) fprintf(fp, "0%d", qtX);
		else fprintf(fp, "00%d", qtX);
	}
	if((ad)==(&bhX))
	{
		fseek(fp, 14, SEEK_SET);
		if(bhX>99) fprintf(fp, "%d", bhX);
		else if(bhX>9) fprintf(fp, "0%d", bhX);
		else fprintf(fp, "00%d", bhX);
	}
	if((ad)==(&X_eats_diag))
	{
		fseek(fp, 19, SEEK_SET);
		fprintf(fp, "%d", X_eats_diag);
	}
	if((ad)==(&X_improved_percep))
	{
		fseek(fp, 22, SEEK_SET);
		fprintf(fp, "%d", X_improved_percep);
	}
	if((ad)==(&X_cross_screen))
	{
		fseek(fp, 25, SEEK_SET);
		fprintf(fp, "%d", X_cross_screen);
	}
	if((ad)==(&qtB))
	{
		fseek(fp, 28, SEEK_SET);
		if(qtB>99) fprintf(fp, "%d", qtB);
		else if(qtB>9) fprintf(fp, "0%d", qtB);
		else fprintf(fp, "00%d", qtB);
	}
	if((ad)==(&trB))
	{
		fseek(fp, 33, SEEK_SET);
		if(trB>99) fprintf(fp, "%d", trB);
		else if(trB>9) fprintf(fp, "0%d", trB);
		else fprintf(fp, "00%d", trB);
	}
	if((ad)==(&qtQ))
	{
		fseek(fp, 38, SEEK_SET);
		if(qtQ>99) fprintf(fp, "%d", qtQ);
		else if(qtQ>9) fprintf(fp, "0%d", qtQ);
		else fprintf(fp, "00%d", qtQ);
	}
	if((ad)==(&cdQ))
	{
		fseek(fp, 43, SEEK_SET);
		if(cdQ>99) fprintf(fp, "%d", cdQ);
		else if(cdQ>9) fprintf(fp, "0%d", cdQ);
		else fprintf(fp, "00%d", cdQ);
	}
	if((ad)==(&tQ_base))
	{
		fseek(fp, 48, SEEK_SET);
		if(tQ_base>9) fprintf(fp, "%d", tQ_base);
		else fprintf(fp, "0%d", tQ_base);
	}
	if((ad)==(&rdQ))
	{
		fseek(fp, 52, SEEK_SET);
		if(rdQ>99) fprintf(fp, "%d", rdQ);
		else if(rdQ>9) fprintf(fp, "0%d", rdQ);
		else fprintf(fp, "00%d", rdQ);
	}
	if((ad)==(&qtO))
	{
		fseek(fp, 57, SEEK_SET);
		if(qtO>99) fprintf(fp, "%d", qtO);
		else if(qtO>9) fprintf(fp, "0%d", qtO);
		else fprintf(fp, "00%d", qtO);
	}

	fclose(fp);
}

void logo () {
	CLEAR;
	printf( YELLOW "\n\n");
	printf( BOLD "      ******            ****     ****                   \n");
	printf("     **" NORMAL YELLOW "////" BOLD "**          " NORMAL YELLOW "/" BOLD "**" NORMAL YELLOW "/" BOLD "**   **" NORMAL YELLOW "/" BOLD "**                   \n");
	printf("    **    " NORMAL YELLOW "//           /" BOLD "**" NORMAL YELLOW "//" BOLD "** ** " NORMAL YELLOW "/" BOLD "**  ******   ******* \n");
	printf( NORMAL YELLOW "   /" BOLD "**          *****  " NORMAL YELLOW "/" BOLD "** " NORMAL YELLOW "//" BOLD "***  " NORMAL YELLOW "/" BOLD "** " NORMAL YELLOW "//////" BOLD "** " NORMAL YELLOW "//" BOLD "**" NORMAL YELLOW "///" BOLD "**\n");
	printf( NORMAL YELLOW "   /" BOLD "**         " NORMAL YELLOW "/////   /" BOLD "**  " NORMAL YELLOW "//" BOLD "*   " NORMAL YELLOW "/" BOLD "**  *******  " NORMAL YELLOW "/" BOLD "**  " NORMAL YELLOW "/" BOLD "**\n");
	printf( NORMAL YELLOW "   //" BOLD "**    **          " NORMAL YELLOW "/" BOLD "**   " NORMAL YELLOW "/    /" BOLD "** **" NORMAL YELLOW "////" BOLD "**  " NORMAL YELLOW "/" BOLD "**  " NORMAL YELLOW "/" BOLD "**\n");
	printf( NORMAL YELLOW "    //" BOLD "******           " NORMAL YELLOW "/" BOLD "**        " NORMAL YELLOW "/" BOLD "**" NORMAL YELLOW "//" BOLD "******** ***  " NORMAL YELLOW "/" BOLD "**\n");
	printf( NORMAL YELLOW "     //////            //         //  //////// ///   // \n\n" NORMAL );
	printf( YELLOW "                ======================\n" NORMAL );
	printf( BOLD "\n                   Pressione <enter>\n" NORMAL );
	getchar();
}

void game_over() {
	printf( RED "\n..####....####...##...##..######...........####...##..##..######..#####..\n");
	printf(".##......##..##..###.###..##..............##..##..##..##..##......##..##.\n");
	printf(".##.###..######..##.#.##..####............##..##..##..##..####....#####..\n");
	printf(".##..##..##..##..##...##..##..............##..##...####...##......##..##.\n");
	printf("..####...##..##..##...##..######...........####.....##....######..##..##.\n");
	printf(".........................................................................\n\n" NORMAL );

	if(murdered) printf(" Voce foi assassinado\n");
	else if(starved) printf(" Voce morreu de fome\n");
	else if(blownup) printf(" Voce foi incinerado\n");
	else if(cornered) printf(" Voce foi encurralado\n");

	printf("\n Sua pontuacao final:"GREEN" %d\n" NORMAL , mag(score));

	printf("\n Aperte <enter> para voltar ao meu principal\n");

	getchar();
}

void menu () {
	int command=0;

	CLEAR;
	printf("\n\n");
	printf( YELLOW " ============================\n");
	printf(" ||   "CIAN BOLD"Escolha uma opcao:   "NORMAL YELLOW"||\n");
	printf(" ============================\n");
	printf(" ||------------------------||\n");
	printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Jogar              "YELLOW"||\n");
	printf(" ||------------------------||\n");
	printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Configuracoes      "YELLOW"||\n");
	printf(" ||------------------------||\n");
	printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Ranking            "YELLOW"||\n");
	printf(" ||------------------------||\n");
	printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Instrucoes         "YELLOW"||\n");
	printf(" ||------------------------||\n");
	printf(" ||"BOLD" 5 "NORMAL YELLOW"- "NORMAL "Sair               "YELLOW"||\n");
	printf(" ||------------------------||\n");
	printf(" ============================\n" NORMAL );
	do
	{
		printf(" ");
		scanf("%d", &command);
		getchar();
	} while(command!=1&&command!=2&&command!=3&&command!=4&&command!=5);
	switch(command)
	{
		case 1: play(); break;
		case 2: config(); break;
		case 3: show_ranking(); break;
		case 4: instruct(); break;
		case 5: running=0;
	}
}

void config() {
	int command=0;
	int confing=1;

	do
	{
		CLEAR;
		printf("\n\n");
		printf( YELLOW " =====================================\n");
		printf(" ||          "CIAN BOLD"Configurar:            "NORMAL YELLOW"||\n");
		printf(" =====================================\n");
		printf(" ||---------------------------------||\n");
		if(!ranked) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Dimensoes tabuleiro "BLACK BOLD"(%dx%d) "NORMAL YELLOW"||\n", wdt, hgt);
		else printf(" ||"BLACK BOLD" 1 - Dimensoes tabuleiro "BLACK BOLD"(%dx%d) "NORMAL YELLOW"||\n", wdt, hgt);
		printf(" ||---------------------------------||\n");
		if(!ranked) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "NPCs                        "YELLOW"||\n");
		else printf(" ||"BLACK BOLD" 2 - NPCs                        "NORMAL YELLOW"||\n");
		printf(" ||---------------------------------||\n");
		if(!ranked)	printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Modo ranqueado              "YELLOW"||\n");
		else printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Modo nao ranqueado          "YELLOW"||\n");
		printf(" ||---------------------------------||\n");
		printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Retornar                    "YELLOW"||\n");
		printf(" ||---------------------------------||\n");
		printf( YELLOW " =====================================\n" NORMAL );
		do
		{
			printf(" ");
			scanf("%d", &command);
			getchar();
		} while(command!=1&&command!=2&&command!=3&&command!=4);
		switch(command)
		{
			case 1:
				if(!ranked) config_board();
				else
				{
					printf( RED BOLD " !Nao e possivel mudar as configuracoes no modo ranqueado!\n" NORMAL );
					printf("\n Pressione <enter> para voltar\n");
					getchar();
				}
				break;
			case 2:
				if(!ranked) config_npcs();
				else
				{
					printf( RED BOLD " !Nao e possivel mudar as configuracoes no modo ranqueado!\n" NORMAL );
					printf("\n Pressione <enter> para voltar\n");
					getchar();
				}
				break;
			case 3: toggle_ranked(); break;
			case 4: confing=0;
		}
	} while(confing);
}

void config_board() {
	int nhgt, nwdt;
	int user_is_trolling=0;

	do
	{
		CLEAR;
		printf("\n\n");
		printf( BLACK BOLD " =====================================\n");
		printf(" ||          Configurar:            ||\n");
		printf(" =====================================\n");
		printf(" ||---------------------------------||\n");
		printf(" || 1 - Dimensoes tabuleiro (%dx%d) ||\n", wdt, hgt);
		printf(" ||---------------------------------||\n");
		printf(" || 2 - NPCs                        ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 3 - Modo ranqueado              ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 4 - Retornar                    ||\n");
		printf(" ||---------------------------------||\n");
		printf(" =====================================\n" NORMAL );
		if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 5 e 100!\n" NORMAL );
		else user_is_trolling=1;
		printf( YELLOW BOLD " \\\\ Informe a nova largura e altura desejadas, respectivamente (minimo 5, maximo 100): " GREEN BOLD );
		printf(" ");
		scanf("%d %d", &nwdt, &nhgt);
		getchar();
	} while((nhgt<5)||(nhgt>100)||(nwdt<5)||(nwdt>100));

	wdt=nwdt;
	hgt=nhgt;
	store(&wdt);
	store(&hgt);

	printf( NORMAL " Dimensoes alteradas com sucesso!\n");


	printf("\n Pressione <enter> para voltar\n");
	getchar();

}

void config_npcs() {
	int command;
	int user_is_trolling=0, confing=1, xconfing, bconfing, qconfing, oconfing;

	do
	{
		CLEAR;
		printf("\n\n");
		printf( YELLOW " =============================\n");
		printf(" ||      "CIAN BOLD"Configurar:        "NORMAL YELLOW"||\n");
		printf(" =============================\n");
		printf(" ||-------------------------||\n");
		printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Personagem X        "YELLOW"||\n");
		printf(" ||-------------------------||\n");
		printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Personagem B        "YELLOW"||\n");
		printf(" ||-------------------------||\n");
		printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Personagem Q        "YELLOW"||\n");
		printf(" ||-------------------------||\n");
		if(qtO>99) printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Quantidade O "BLACK BOLD"(%d)  "NORMAL YELLOW"||\n", qtO);
		else if(qtO>9) printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Quantidade O "BLACK BOLD"(%d)   "NORMAL YELLOW"||\n", qtO);
		else  printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Quantidade O "BLACK BOLD"(%d)    "NORMAL YELLOW"||\n", qtO);
		printf(" ||-------------------------||\n");
		printf(" ||"BOLD" 5 "NORMAL YELLOW"- "NORMAL "Retornar            "YELLOW"||\n");
		printf(" ||-------------------------||\n");
		printf( YELLOW " =============================\n" NORMAL );
		do
		{
			printf(" ");
			scanf("%d", &command);
			getchar();
		} while(command!=1&&command!=2&&command!=3&&command!=4&&command!=5);
		switch(command)
		{
			case 1: xconfing=1; do
			{
				CLEAR;
				printf("\n\n");
				printf( YELLOW " =============================\n");
				printf(" ||      "CIAN BOLD"Configurar X:      "NORMAL YELLOW"||\n");
				printf(" =============================\n");
				printf(" ||-------------------------||\n");
				if(qtX>99) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)    "NORMAL YELLOW"||\n", qtX);
				else if(qtX>9) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)     "NORMAL YELLOW"||\n", qtX);
				else printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)      "NORMAL YELLOW"||\n", qtX);
				printf(" ||-------------------------||\n");
				if(bhX>99) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Agressividade "BLACK BOLD"(%d) "NORMAL YELLOW"||\n", bhX);
				else if(bhX>9) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Agressividade "BLACK BOLD"(%d)  "NORMAL YELLOW"||\n", bhX);
				else printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Agressividade "BLACK BOLD"(%d)   "NORMAL YELLOW"||\n", bhX);
				printf(" ||-------------------------||\n");
				printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Inteligencia "BLACK BOLD"(%d %d %d)"NORMAL YELLOW"||\n", X_eats_diag, X_improved_percep, X_cross_screen);
				printf(" ||-------------------------||\n");
				printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Retornar            "NORMAL YELLOW"||\n");
				printf(" ||-------------------------||\n");
				printf( YELLOW " =============================\n" NORMAL );
				do {printf(" ");
					scanf("%d", &command); getchar();} while(command!=1&&command!=2&&command!=3&&command!=4);
				switch(command)
				{
					case 1: user_is_trolling=0;
						int nqtX;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =============================\n");
							printf(" ||      Configurar X:      ||\n");
							printf(" =============================\n");
							printf(" ||-------------------------||\n");
							if(qtX>99) printf(" || 1 - Quantidade (%d)    ||\n", qtX);
							else if(qtX>9) printf(" || 1 - Quantidade (%d)     ||\n", qtX);
							else printf(" || 1 - Quantidade (%d)      ||\n", qtX);
							printf(" ||-------------------------||\n");
							if(bhX>99) printf(" || 2 - Agressividade (%d) ||\n", bhX);
							else if(bhX>9) printf(" || 2 - Agressividade (%d)  ||\n", bhX);
							else printf(" || 2 - Agressividade (%d)   ||\n", bhX);
							printf(" ||-------------------------||\n");
							printf(" || 3 - Inteligencia (%d %d %d)||\n", X_eats_diag, X_improved_percep, X_cross_screen);
							printf(" ||-------------------------||\n");
							printf(" || 4 - Retornar            ||\n");
							printf(" ||-------------------------||\n");
							printf(" =============================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 0 e 150!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe a nova quantidade desejada (minimo 0, maximo 150): " GREEN BOLD );
							printf(" ");
							scanf("%d", &nqtX);
							getchar();
						} while((nqtX<0)||(nqtX>150));

						qtX=nqtX;
						store(&qtX);

						printf( NORMAL " Quantidade alterada com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 2: user_is_trolling=0;
						int nbhX;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =============================\n");
							printf(" ||      Configurar X:      ||\n");
							printf(" =============================\n");
							printf(" ||-------------------------||\n");
							if(qtX>99) printf(" || 1 - Quantidade (%d)    ||\n", qtX);
							else if(qtX>9) printf(" || 1 - Quantidade (%d)     ||\n", qtX);
							else printf(" || 1 - Quantidade (%d)      ||\n", qtX);
							printf(" ||-------------------------||\n");
							if(bhX>99) printf(" || 2 - Agressividade (%d) ||\n", bhX);
							else if(bhX>9) printf(" || 2 - Agressividade (%d)  ||\n", bhX);
							else printf(" || 2 - Agressividade (%d)   ||\n", bhX);
							printf(" ||-------------------------||\n");
							printf(" || 3 - Inteligencia (%d %d %d)||\n", X_eats_diag, X_improved_percep, X_cross_screen);
							printf(" ||-------------------------||\n");
							printf(" || 4 - Retornar            ||\n");
							printf(" ||-------------------------||\n");
							printf(" =============================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 0 e 999!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe por quantos turnos o X devera permanecer passivo (minimo 0, maximo 999): " GREEN BOLD );
							printf(" ");
							scanf("%d", &nbhX);
							getchar();
						} while((nbhX<0)||(nbhX>999));

						bhX=nbhX;
						store(&bhX);

						printf( NORMAL " Agressividade alterada com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 3:
						int user_cant_type=0;
						char ans[100];
						
						CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =============================\n");
							printf(" ||      Configurar X:      ||\n");
							printf(" =============================\n");
							printf(" ||-------------------------||\n");
							if(qtX>99) printf(" || 1 - Quantidade (%d)    ||\n", qtX);
							else if(qtX>9) printf(" || 1 - Quantidade (%d)     ||\n", qtX);
							else printf(" || 1 - Quantidade (%d)      ||\n", qtX);
							printf(" ||-------------------------||\n");
							if(bhX>99) printf(" || 2 - Agressividade (%d) ||\n", bhX);
							else if(bhX>9) printf(" || 2 - Agressividade (%d)  ||\n", bhX);
							else printf(" || 2 - Agressividade (%d)   ||\n", bhX);
							printf(" ||-------------------------||\n");
							printf(" || 3 - Inteligencia (%d %d %d)||\n", X_eats_diag, X_improved_percep, X_cross_screen);
							printf(" ||-------------------------||\n");
							printf(" || 4 - Retornar            ||\n");
							printf(" ||-------------------------||\n");
							printf(" =============================\n" NORMAL );
						do
						{
							if(user_cant_type) printf( RED BOLD " !Digite s ou n !\n" NORMAL );
							printf( YELLOW BOLD " \\\\ Informe se o X deve comer na diagonal (como um peao): [s/n] " GREEN BOLD );
							printf(" ");
							scanf("%[^\n]s", ans);
							getchar();
							if(((ans[0]!='n')&&(ans[0]!='s')&&(ans[0]!='S')&&(ans[0]!='N'))||(ans[1]!='\0')) user_cant_type=1;
							else user_cant_type=0;
						} while(user_cant_type);

						if((ans[0]=='s')||(ans[0]=='S')) X_eats_diag=1;
						else X_eats_diag=0;
						store(&X_eats_diag);
						user_cant_type=0;

						do
						{
							if(user_cant_type) printf( RED BOLD " !Digite s ou n !\n" NORMAL );
							printf( YELLOW BOLD " \\\\ Informe se o X deve ser mais eficaz em perseguir o C: [s/n] " GREEN BOLD );
							printf(" ");
							scanf("%[^\n]s", ans);
							getchar();
							if(((ans[0]!='n')&&(ans[0]!='s')&&(ans[0]!='S')&&(ans[0]!='N'))||(ans[1]!='\0')) user_cant_type=1;
							else user_cant_type=0;
						} while(user_cant_type);

						if((ans[0]=='s')||(ans[0]=='S')) X_improved_percep=1;
						else X_improved_percep=0;
						store(&X_improved_percep);
						user_cant_type=0;

						do
						{
							if(user_cant_type) printf( RED BOLD " !Digite s ou n !\n" NORMAL );
							printf( YELLOW BOLD " \\\\ Informe se o X deve ser capaz de atravessar as bordas da tela: [s/n] " GREEN BOLD );
							printf(" ");
							scanf("%[^\n]s", ans);
							getchar();
							if(((ans[0]!='n')&&(ans[0]!='s')&&(ans[0]!='S')&&(ans[0]!='N'))||(ans[1]!='\0')) user_cant_type=1;
							else user_cant_type=0;
						} while(user_cant_type);

						if((ans[0]=='s')||(ans[0]=='S')) X_cross_screen=1;
						else X_cross_screen=0;
						store(&X_cross_screen);

						printf( NORMAL " Inteligencia alterada com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 4: xconfing=0;
				}
			} while(xconfing);
			break;
			case 2: bconfing=1; do
			{
				CLEAR;
				printf("\n\n");
				printf( YELLOW " =============================\n");
				printf(" ||      "CIAN BOLD"Configurar B:      "NORMAL YELLOW"||\n");
				printf(" =============================\n");
				printf(" ||-------------------------||\n");
				if(qtB>99) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)    "NORMAL YELLOW"||\n", qtB);
				else if(qtB>9) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)     "NORMAL YELLOW"||\n", qtB);
				else printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)      "NORMAL YELLOW"||\n", qtB);
				printf(" ||-------------------------||\n");
				if(trB>99) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Rastro "BLACK BOLD"(%d)        "NORMAL YELLOW"||\n", trB);
				else if(trB>9) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Rastro "BLACK BOLD"(%d)         "NORMAL YELLOW"||\n", trB);
				else printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Rastro "BLACK BOLD"(%d)          "NORMAL YELLOW"||\n", trB);
				printf(" ||-------------------------||\n");
				printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Retornar            "YELLOW"||\n");
				printf(" ||-------------------------||\n");
				printf( YELLOW " =============================\n" NORMAL );
				do {printf(" ");
					scanf("%d", &command); getchar();} while(command!=1&&command!=2&&command!=3);
				switch(command)
				{
					case 1: user_is_trolling=0;
						int nqtB;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =============================\n");
							printf(" ||      Configurar B:      ||\n");
							printf(" =============================\n");
							printf(" ||-------------------------||\n");
							if(qtB>99) printf(" || 1 - Quantidade (%d)    ||\n", qtB);
							else if(qtB>9) printf(" || 1 - Quantidade (%d)     ||\n", qtB);
							else printf(" || 1 - Quantidade (%d)      ||\n", qtB);
							printf(" ||-------------------------||\n");
							if(trB>99) printf(" || 2 - Rastro (%d)        ||\n", trB);
							else if(trB>9) printf(" || 2 - Rastro (%d)         ||\n", trB);
							else printf(" || 2 - Rastro (%d)          ||\n", trB);
							printf(" ||-------------------------||\n");
							printf(" || 3 - Retornar            ||\n");
							printf(" ||-------------------------||\n");
							printf(" =============================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 0 e 100!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe a nova quantidade desejada (minimo 0, maximo 100): " GREEN BOLD );
							printf(" ");
							scanf("%d", &nqtB);
							getchar();
						} while((nqtB<0)||(nqtB>100));

						qtB=nqtB;
						store(&qtB);

						printf( NORMAL " Quantidade alterada com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 2: user_is_trolling=0;
						int ntrB;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =============================\n");
							printf(" ||      Configurar B:      ||\n");
							printf(" =============================\n");
							printf(" ||-------------------------||\n");
							if(qtB>99) printf(" || 1 - Quantidade (%d)    ||\n", qtB);
							else if(qtB>9) printf(" || 1 - Quantidade (%d)     ||\n", qtB);
							else printf(" || 1 - Quantidade (%d)      ||\n", qtB);
							printf(" ||-------------------------||\n");
							if(trB>99) printf(" || 2 - Rastro (%d)        ||\n", trB);
							else if(trB>9) printf(" || 2 - Rastro (%d)         ||\n", trB);
							else printf(" || 2 - Rastro (%d)          ||\n", trB);
							printf(" ||-------------------------||\n");
							printf(" || 3 - Retornar            ||\n");
							printf(" ||-------------------------||\n");
							printf(" =============================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 1 e 100!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe o novo tamanho do B (minimo 1, maximo 100): " GREEN BOLD );
							printf(" ");
							scanf("%d", &ntrB);
							getchar();
						} while((ntrB<1)||(ntrB>100));

						trB=ntrB;
						store(&trB);

						printf( NORMAL " Tamanho alterado com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 3: bconfing=0;
				}
			} while(bconfing);
			break;
			case 3: qconfing=1; do
			{
				CLEAR;
				printf("\n\n");
				printf( YELLOW " =======================================\n");
				printf(" ||          "CIAN BOLD"Configurar Q:            "NORMAL YELLOW"||\n");
				printf(" =======================================\n");
				printf(" ||-----------------------------------||\n");
				if(qtQ>99) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)              "NORMAL YELLOW"||\n", qtQ);
				else if(qtQ>9) printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)               "NORMAL YELLOW"||\n", qtQ);
				else printf(" ||"BOLD" 1 "NORMAL YELLOW"- "NORMAL "Quantidade "BLACK BOLD"(%d)                "NORMAL YELLOW"||\n", qtQ);
				printf(" ||-----------------------------------||\n");
				if(cdQ>99) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Tempo de recarga "BLACK BOLD"(%d)        "NORMAL YELLOW"||\n", cdQ);
				else if(cdQ>9) printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Tempo de recarga "BLACK BOLD"(%d)         "NORMAL YELLOW"||\n", cdQ);
				else printf(" ||"BOLD" 2 "NORMAL YELLOW"- "NORMAL "Tempo de recarga "BLACK BOLD"(%d)          "NORMAL YELLOW"||\n", cdQ);
				printf(" ||-----------------------------------||\n");
				if(tQ_base>9) printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Tempo medio de explosao "BLACK BOLD"(%d)  "NORMAL YELLOW"||\n", tQ_base);
				else printf(" ||"BOLD" 3 "NORMAL YELLOW"- "NORMAL "Tempo medio de explosao "BLACK BOLD"(%d)   "NORMAL YELLOW"||\n", tQ_base);
				printf(" ||-----------------------------------||\n");
				if(rdQ>99) printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Alcance "BLACK BOLD"(%d)                 "NORMAL YELLOW"||\n", rdQ);
				else if(rdQ>9) printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Alcance "BLACK BOLD"(%d)                  "NORMAL YELLOW"||\n", rdQ);
				else printf(" ||"BOLD" 4 "NORMAL YELLOW"- "NORMAL "Alcance "BLACK BOLD"(%d)                   "NORMAL YELLOW"||\n", rdQ);
				printf(" ||-----------------------------------||\n");
				printf(" ||"BOLD" 5 "NORMAL YELLOW"- "NORMAL "Retornar                      "NORMAL YELLOW"||\n");
				printf(" ||-----------------------------------||\n");
				printf( YELLOW " =======================================\n" NORMAL );
				do {printf(" ");
					scanf("%d", &command); getchar();} while(command!=1&&command!=2&&command!=3&&command!=4&&command!=5);
				switch(command)
				{
					case 1: user_is_trolling=0;
						int nqtQ;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =======================================\n");
							printf(" ||          Configurar Q:            ||\n");
							printf(" =======================================\n");
							printf(" ||-----------------------------------||\n");
							if(qtQ>99) printf(" || 1 - Quantidade (%d)              ||\n", qtQ);
							else if(qtQ>9) printf(" || 1 - Quantidade (%d)               ||\n", qtQ);
							else printf(" || 1 - Quantidade (%d)                ||\n", qtQ);
							printf(" ||-----------------------------------||\n");
							if(cdQ>99) printf(" || 2 - Tempo de recarga (%d)        ||\n", cdQ);
							else if(cdQ>9) printf(" || 2 - Tempo de recarga (%d)         ||\n", cdQ);
							else printf(" || 2 - Tempo de recarga (%d)          ||\n", cdQ);
							printf(" ||-----------------------------------||\n");
							if(tQ_base>9) printf(" || 3 - Tempo medio de explosao (%d)  ||\n", tQ_base);
							else printf(" || 3 - Tempo medio de explosao (%d)   ||\n", tQ_base);
							printf(" ||-----------------------------------||\n");
							if(rdQ>99) printf(" || 4 - Alcance (%d)                 ||\n", rdQ);
							else if(rdQ>9) printf(" || 4 - Alcance (%d)                  ||\n", rdQ);
							else printf(" || 4 - Alcance (%d)                   ||\n", rdQ);
							printf(" ||-----------------------------------||\n");
							printf(" || 5 - Retornar                      ||\n");
							printf(" ||-----------------------------------||\n");
							printf(" =======================================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 0 e 100!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe a nova quantidade maxima desejada (minimo 0, maximo 100): " GREEN BOLD );
							printf(" ");
							scanf("%d", &nqtQ);
							getchar();
						} while((nqtQ<0)||(nqtQ>100));

						qtQ=nqtQ;
						store(&qtQ);

						printf( NORMAL " Quantidade alterada com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 2: user_is_trolling=0;
						int ncdQ;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =======================================\n");
							printf(" ||          Configurar Q:            ||\n");
							printf(" =======================================\n");
							printf(" ||-----------------------------------||\n");
							if(qtQ>99) printf(" || 1 - Quantidade (%d)              ||\n", qtQ);
							else if(qtQ>9) printf(" || 1 - Quantidade (%d)               ||\n", qtQ);
							else printf(" || 1 - Quantidade (%d)                ||\n", qtQ);
							printf(" ||-----------------------------------||\n");
							if(cdQ>99) printf(" || 2 - Tempo de recarga (%d)        ||\n", cdQ);
							else if(cdQ>9) printf(" || 2 - Tempo de recarga (%d)         ||\n", cdQ);
							else printf(" || 2 - Tempo de recarga (%d)          ||\n", cdQ);
							printf(" ||-----------------------------------||\n");
							if(tQ_base>9) printf(" || 3 - Tempo medio de explosao (%d)  ||\n", tQ_base);
							else printf(" || 3 - Tempo medio de explosao (%d)   ||\n", tQ_base);
							printf(" ||-----------------------------------||\n");
							if(rdQ>99) printf(" || 4 - Alcance (%d)                 ||\n", rdQ);
							else if(rdQ>9) printf(" || 4 - Alcance (%d)                  ||\n", rdQ);
							else printf(" || 4 - Alcance (%d)                   ||\n", rdQ);
							printf(" ||-----------------------------------||\n");
							printf(" || 5 - Retornar                      ||\n");
							printf(" ||-----------------------------------||\n");
							printf(" =======================================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 1 e 100!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe o novo tempo de recarga da bomba (minimo 1, maximo 100): " GREEN BOLD );
							printf(" ");
							scanf("%d", &ncdQ);
							getchar();
						} while((ncdQ<1)||(ncdQ>100));

						cdQ=ncdQ;
						store(&cdQ);

						printf( NORMAL " Tempo de recarga alterado com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 3: user_is_trolling=0;
						int ntQ_base;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =======================================\n");
							printf(" ||          Configurar Q:            ||\n");
							printf(" =======================================\n");
							printf(" ||-----------------------------------||\n");
							if(qtQ>99) printf(" || 1 - Quantidade (%d)              ||\n", qtQ);
							else if(qtQ>9) printf(" || 1 - Quantidade (%d)               ||\n", qtQ);
							else printf(" || 1 - Quantidade (%d)                ||\n", qtQ);
							printf(" ||-----------------------------------||\n");
							if(cdQ>99) printf(" || 2 - Tempo de recarga (%d)        ||\n", cdQ);
							else if(cdQ>9) printf(" || 2 - Tempo de recarga (%d)         ||\n", cdQ);
							else printf(" || 2 - Tempo de recarga (%d)          ||\n", cdQ);
							printf(" ||-----------------------------------||\n");
							if(tQ_base>9) printf(" || 3 - Tempo medio de explosao (%d)  ||\n", tQ_base);
							else printf(" || 3 - Tempo medio de explosao (%d)   ||\n", tQ_base);
							printf(" ||-----------------------------------||\n");
							if(rdQ>99) printf(" || 4 - Alcance (%d)                 ||\n", rdQ);
							else if(rdQ>9) printf(" || 4 - Alcance (%d)                  ||\n", rdQ);
							else printf(" || 4 - Alcance (%d)                   ||\n", rdQ);
							printf(" ||-----------------------------------||\n");
							printf(" || 5 - Retornar                      ||\n");
							printf(" ||-----------------------------------||\n");
							printf(" =======================================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 3 e 25!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe o novo tempo medio de explosao da bomba (minimo 3, maximo 25): " GREEN BOLD );
							printf(" ");
							scanf("%d", &ntQ_base);
							getchar();
						} while((ntQ_base<3)||(ntQ_base>25));

						tQ_base=ntQ_base;
						store(&tQ_base);

						printf( NORMAL " Tempo medio de explosao alterado com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 4: user_is_trolling=0;
						int nrdQ;
						do
						{
							CLEAR;
							printf("\n\n");
							printf( BLACK BOLD " =======================================\n");
							printf(" ||          Configurar Q:            ||\n");
							printf(" =======================================\n");
							printf(" ||-----------------------------------||\n");
							if(qtQ>99) printf(" || 1 - Quantidade (%d)              ||\n", qtQ);
							else if(qtQ>9) printf(" || 1 - Quantidade (%d)               ||\n", qtQ);
							else printf(" || 1 - Quantidade (%d)                ||\n", qtQ);
							printf(" ||-----------------------------------||\n");
							if(cdQ>99) printf(" || 2 - Tempo de recarga (%d)        ||\n", cdQ);
							else if(cdQ>9) printf(" || 2 - Tempo de recarga (%d)         ||\n", cdQ);
							else printf(" || 2 - Tempo de recarga (%d)          ||\n", cdQ);
							printf(" ||-----------------------------------||\n");
							if(tQ_base>9) printf(" || 3 - Tempo medio de explosao (%d)  ||\n", tQ_base);
							else printf(" || 3 - Tempo medio de explosao (%d)   ||\n", tQ_base);
							printf(" ||-----------------------------------||\n");
							if(rdQ>99) printf(" || 4 - Alcance (%d)                 ||\n", rdQ);
							else if(rdQ>9) printf(" || 4 - Alcance (%d)                  ||\n", rdQ);
							else printf(" || 4 - Alcance (%d)                   ||\n", rdQ);
							printf(" ||-----------------------------------||\n");
							printf(" || 5 - Retornar                      ||\n");
							printf(" ||-----------------------------------||\n");
							printf(" =======================================\n" NORMAL );
							if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 1 e 100!\n" NORMAL );
							else user_is_trolling=1;
							printf( YELLOW BOLD " \\\\ Informe o novo alcance da explosao (minimo 1, maximo 100): " GREEN BOLD );
							printf(" ");
							scanf("%d", &nrdQ);
							getchar();
						} while((nrdQ<1)||(nrdQ>100));

						rdQ=nrdQ;
						store(&rdQ);

						printf( NORMAL " Alcance da explosao alterado com sucesso!\n");

						printf("\n Pressione <enter> para voltar\n");
						getchar();
					break;
					case 5: qconfing=0;
				}
			} while(qconfing);
			break;
			case 4: user_is_trolling=0;
				int nqtO;
				do
				{
					CLEAR;
					printf("\n\n");
					printf( BLACK BOLD " =============================\n");
					printf(" ||      Configurar:        ||\n");
					printf(" =============================\n");
					printf(" ||-------------------------||\n");
					printf(" || 1 - Personagem X        ||\n");
					printf(" ||-------------------------||\n");
					printf(" || 2 - Personagem B        ||\n");
					printf(" ||-------------------------||\n");
					printf(" || 3 - Personagem Q        ||\n");
					printf(" ||-------------------------||\n");
					if(qtO>99) printf(" || 4 - Quantidade O (%d)  ||\n", qtO);
					else if(qtO>9) printf(" || 4 - Quantidade O (%d)   ||\n", qtO);
					else printf(" || 4 - Quantidade O (%d)    ||\n", qtO);
					printf(" ||-------------------------||\n");
					printf(" || 5 - Retornar            ||\n");
					printf(" ||-------------------------||\n");
					printf(" =============================\n" NORMAL );
					if(user_is_trolling) printf( RED BOLD " !Escolha valores entre 0 e 200!\n" NORMAL );
					else user_is_trolling=1;
					printf( YELLOW BOLD " \\\\ Informe a nova quantidade desejada (minimo 0, maximo 200): " GREEN BOLD );
					printf(" ");
					scanf("%d", &nqtO);
					getchar();
				} while((nqtO<0)||(nqtO>200));

				qtO=nqtO;
				store(&qtO);

				printf( NORMAL " Quantidade alterada com sucesso!\n");

				printf("\n Pressione <enter> para voltar\n");
				getchar();
			break;
			case 5: confing=0;
		}
	} while(confing);
	
}

void toggle_ranked() {
	int user_cant_type=0;
	char ans[101];
	if(!ranked)
	{
		CLEAR;
		printf("\n\n");
		printf( BLACK BOLD " =====================================\n");
		printf(" ||          Configurar:            ||\n");
		printf(" =====================================\n");
		printf(" ||---------------------------------||\n");
		printf(" || 1 - Dimensoes tabuleiro (%dx%d) ||\n", wdt, hgt);
		printf(" ||---------------------------------||\n");
		printf(" || 2 - NPCs                        ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 3 - Modo ranqueado              ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 4 - Retornar                    ||\n");
		printf(" ||---------------------------------||\n");
		printf(" =====================================\n" NORMAL );
		

		do
		{
			if(user_cant_type) printf( RED BOLD " !Digite s ou n !\n" NORMAL );
			printf(" \\\\Deseja realmente ativar o modo ranqueado? [s/n]\n");
			printf(" Todas as configuracoes de jogo serao alteradas para o modo de configuracoes ranqueado.\n");
			printf(" ");
			scanf("%[^\n]s", ans);
			getchar();
			if(((ans[0]!='n')&&(ans[0]!='s')&&(ans[0]!='S')&&(ans[0]!='N'))||(ans[1]!='\0')) user_cant_type=1;
			else user_cant_type=0;
		} while(user_cant_type);

		if((ans[0]=='s')||(ans[0]=='S'))
		{
			ranked=1;
			hgt=30;
			wdt=30;
			qtB=3;
			qtX=7;
			trB=7;
			rdQ=10;
			bhX=7;
			difficulty=2;
			qtO=10;
			qtQ=1;
			cdQ=10;
			tQ_base=6;
		}
	}
	else
	{
		ranked=0;
		difficulty=-1;
		loadall();
		CLEAR;
		printf("\n\n");
		printf( BLACK BOLD " =====================================\n");
		printf(" ||          Configurar:            ||\n");
		printf(" =====================================\n");
		printf(" ||---------------------------------||\n");
		printf(" || 1 - Dimensoes tabuleiro (%dx%d) ||\n", wdt, hgt);
		printf(" ||---------------------------------||\n");
		printf(" || 2 - NPCs                        ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 3 - Modo ranqueado              ||\n");
		printf(" ||---------------------------------||\n");
		printf(" || 4 - Retornar                    ||\n");
		printf(" ||---------------------------------||\n");
		printf(" =====================================\n" NORMAL );
		printf(" Modo ranqueado desabilitado!\n");

		printf("\n Pressione <enter> para voltar\n");
		getchar();
	}
}

void show_ranking() {
	Rank player;
	FILE* fp;

	fp=fopen("ranking.bin", "rb");

	if(!fp)
	{
		CLEAR;
		printf("\n\n");
		printf( BLACK BOLD " ============================\n");
		printf(" ||   Escolha uma opcao:   ||\n");
		printf(" ============================\n");
		printf(" ||------------------------||\n");
		printf(" || 1 - Jogar              ||\n");
		printf(" ||------------------------||\n");
		printf(" || 2 - Configuracoes      ||\n");
		printf(" ||------------------------||\n");
		printf(" || 3 - Ranking            ||\n");
		printf(" ||------------------------||\n");
		printf(" || 4 - Instrucoes         ||\n");
		printf(" ||------------------------||\n");
		printf(" || 5 - Sair               ||\n");
		printf(" ||------------------------||\n");
		printf(" ============================\n" NORMAL );
		printf(" Parece que ainda nao existem rankings salvos na pasta do jogo!\n");
		printf("\n Pressione <enter> para voltar\n");
		getchar();
	}
	else
	{
		int i=1, j;

		CLEAR;
		printf("\n\n");
		printf( YELLOW " ==========================================\n");
		printf(" ||                "CIAN BOLD"Ranques               "NORMAL YELLOW"||\n");
		printf(" ==========================================\n");
		printf(" ||--------------------------------------||\n");
		while(fread(&player, 1, sizeof(player), fp))
		{
			if(player.name[0]=='\0') break;

			for(j=0; j<10; j++) if(player.name[j]=='\0') /* extende o nick para que ele sempre ocupe 10 chars */
			{
				player.name[j]=' ';
				player.name[j+1]='\0';
			}
			if(i!=10)
			{
				if(player.hscore>9999) printf(" || "BOLD"Ranque %d  "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"%d "NORMAL YELLOW"||\n", i, player.name, player.hscore);
				else if(player.hscore>999) printf(" || "BOLD"Ranque %d  "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN" %d "NORMAL YELLOW"||\n", i, player.name, player.hscore);
				else if(player.hscore>99) printf(" || "BOLD"Ranque %d  "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"  %d "NORMAL YELLOW"||\n", i, player.name, player.hscore);
				else if(player.hscore>9) printf(" || "BOLD"Ranque %d  "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"   %d "NORMAL YELLOW"||\n", i, player.name, player.hscore);
				else printf(" || "BOLD"Ranque %d  "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"    %d "NORMAL YELLOW"||\n", i, player.name, player.hscore);
			}
			else
			{
				if(player.hscore>9999) printf(" || "BOLD"Ranque 10 "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"%d "NORMAL YELLOW"||\n", player.name, player.hscore);
				else if(player.hscore>999) printf(" || "BOLD"Ranque 10 "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN" %d "NORMAL YELLOW"||\n", player.name, player.hscore);
				else if(player.hscore>99) printf(" || "BOLD"Ranque 10 "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"  %d "NORMAL YELLOW"||\n", player.name, player.hscore);
				else if(player.hscore>9) printf(" || "BOLD"Ranque 10 "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"   %d "NORMAL YELLOW"||\n", player.name, player.hscore);
				else printf(" || "BOLD"Ranque 10 "NORMAL YELLOW"- "NORMAL BOLD"%s "NORMAL "pontos: "GREEN"    %d "NORMAL YELLOW"||\n", player.name, player.hscore);
			}
			printf(" ||--------------------------------------||\n");
			i++;
		}		
		fclose(fp);
		printf(" ==========================================\n" NORMAL );
		printf("\n Pressione <enter> para voltar\n");
		getchar();
	}
}

void update_ranking(Rank player) {
	Rank table[10];
	FILE* fp;
	int i, j;

	player.hscore=score;

	fp=fopen("ranking.bin", "rb");
	if(!fp)
	{
		table[0]=player;
		for(i=1; i<10; i++)
		{
			table[i].name[0]='\0';
			table[i].hscore=0;
		}
		fp=fopen("ranking.bin", "wb");
		fwrite(table, 10, sizeof(Rank), fp);
		fclose(fp);
	}
	else
	{
		int rewrite=0;

		for(i=0; i<10; i++)	fread(&table[i], 1, sizeof(Rank), fp);
		fclose(fp);

		for(i=0; (i<10)&&(!rewrite); i++)
		{
			if(table[i].name[0]=='\0')
			{
				rewrite=1;
				table[i]=player;
			}
			else if(player.hscore>table[i].hscore)
			{
				rewrite=1;
				for(j=0; j<9-i; j++) table[9-j]=table[8-j];
				table[i]=player;
			}
		}
		if(rewrite)
		{
			fp=fopen("ranking.bin", "wb");
			fwrite(table, 10, sizeof(Rank), fp);
			fclose(fp);
		}
	}
}


void instruct() {
	CLEAR;
	printf("\n\n");
	printf( YELLOW " ========================================================\n");
	printf(" ||                     "CIAN BOLD"Instrucoes                     "NORMAL YELLOW"||\n");
	printf( YELLOW " ========================================================\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || "BOLD"Dos personagens:                                  "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"\"C\"                                            "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  E o personagem controlado pelo jogador, que co- "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"leta pontos e desvia de inimigos e explosoes.     "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Controle: teclas \""YELLOW BOLD"w, a, s, d"NORMAL"\".                  "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"\""MAGENTA BOLD"X"YELLOW BOLD"\"                                            "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  E o inimigo principal do jogo, que alterna entre"NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"passivo ("MAGENTA"escuro"NORMAL") e agressivo ("MAGENTA BOLD"destacado"NORMAL").         "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Quando agressivo, persegue ativamente o \"C\", com"NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"o intuito de se mover sobre ele. "YELLOW BOLD"Quando o \"X\" come"NORMAL YELLOW" ||\n");
	printf(" || "YELLOW BOLD"o \"C\", o jogo acaba.                              "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Quando passivo, se movimenta aleatoriamente pelo"NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"mapa.                                             "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||                                                    ||\n");
	printf(" || "BLACK BOLD" . "NORMAL"<-"MAGENTA"X"BLACK BOLD" . . . . . . . . . . . . . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf(" || "BLACK BOLD" . . . . . . . . . "MAGENTA BOLD"X"NORMAL"->"BLACK BOLD" . . . "YELLOW BOLD"C"BLACK BOLD" . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf(" || "BLACK BOLD" . . . . "MAGENTA BOLD"X"NORMAL"->"BLACK BOLD" . . . . . . . . . . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf( YELLOW " ||                                                    ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"\""CIAN BOLD"Q"YELLOW BOLD"\"                                            "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  E uma bomba, que apos alguns turnos, explode,   "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"cobrindo os dois eixos com chamas.                "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Suas chamas incineram todos os personagens (sal-"NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"va a barreira) "YELLOW BOLD"e explodem outras bombas.          "NORMAL YELLOW" ||\n");
	printf(" || "BOLD"  Se a explosao atingir o \"C\", o jogo acaba.      "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||                                                    ||\n");
	printf(" || "BLACK BOLD" . . . . . . . .         "NORMAL"\\"BLACK BOLD"         . "NORMAL RED"#"BLACK BOLD" . . . . . ."NORMAL YELLOW" ||\n");
	printf(" || "BLACK BOLD" . "CIAN BOLD"Q"BLACK BOLD" . "YELLOW BOLD"C"NORMAL"->"BLACK BOLD" . . .        "NORMAL"==)"BLACK BOLD"        "NORMAL RED"# "RED BOLD"! "NORMAL RED"# # "RED BOLD"#"NORMAL RED" # #"BLACK BOLD" ."NORMAL YELLOW" ||\n");
	printf(" || "BLACK BOLD" . . . . . . . .         "NORMAL"/"BLACK BOLD"         . "NORMAL RED"#"BLACK BOLD" . . . . . ."NORMAL YELLOW" ||\n");
	printf( YELLOW " ||                                                    ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"\""NORMAL YELLOW"B"BOLD"\"                                            "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  E um personagem neutro, que se movimenta aleato-"NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"riamente, deixando um rastro para tras.           "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Atua como uma barreira, tanto para o \"C\" quanto "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"para outros personagens.                          "NORMAL YELLOW" ||\n");
 	printf(" || "NORMAL"  Com excecao de sua cabeca, que e vulneravel, "YELLOW BOLD"e  "NORMAL YELLOW" ||\n");
 	printf(" || "YELLOW BOLD"capaz de bloquear o raio de explosao da bomba.    "NORMAL YELLOW" ||\n");
 	printf(" || "YELLOW BOLD"  Se o \"C\" for encurralado pelo \"B\", o jogo acaba."NORMAL YELLOW" ||\n");
 	printf( YELLOW " ||                                                    ||\n");
 	printf(" || "BLACK BOLD" . . "NORMAL RED"#"BLACK BOLD" . . . . . . "NORMAL"B B B B"BLACK BOLD" . . . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf(" || "RED" # # "BOLD"! "NORMAL RED"# # # # # # "NORMAL"B"BLACK BOLD" . "YELLOW BOLD"C "NORMAL"B "YELLOW"B"NORMAL"->"BLACK BOLD" . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf(" || "BLACK BOLD" . . "NORMAL RED"#"BLACK BOLD" . . . . . "NORMAL"B B"BLACK BOLD" . . . . . . . . . . . . . . ."NORMAL YELLOW" ||\n");
	printf( YELLOW " ||                                                    ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || "BOLD"Das pontuacoes:                                   "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"Medidor de energia                             "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Mede quantos movimentos o \"C\" ainda pode fazer  "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"antes de morrer de fome.                          "NORMAL YELLOW" ||\n");
	printf(" || "BOLD"  Quando esgotado, o jogo acaba.                  "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"\""NORMAL CIAN"O"YELLOW BOLD"\"                                            "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Sao pontos espalhados, a comida do \"C\".         "NORMAL YELLOW" ||\n");
	printf(" || "NORMAL"  Quando coletados, somam a pontuacao da partida "YELLOW BOLD"e"NORMAL YELLOW" ||\n");
	printf(" || "BOLD"restituem uma parcela do medidor de energia.      "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || "BOLD"Dos detalhes:                                     "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf(" || => "BOLD"Aos personagens e permitido o movimento atraves"NORMAL YELLOW" ||\n");
	printf(" || "BOLD"da tela, ou seja, eles se teleportam de uma extre-"NORMAL YELLOW" ||\n");
	printf(" || "BOLD"midade ate a outra oposta.                        "NORMAL YELLOW" ||\n");
	printf(" || => "BOLD"Ao morrer, os personagens ou reaparecem imedia-"NORMAL YELLOW" ||\n");
	printf(" || "BOLD"tamente ou apos um pequeno intervalo de turnos.   "NORMAL YELLOW" ||\n");
	printf(" || => "BOLD"Se encurralados, os personagens tem sua posicao"NORMAL YELLOW" ||\n");
	printf(" || "BOLD"no mapa redefinida.                               "NORMAL YELLOW" ||\n");
	printf( YELLOW " ||----------------------------------------------------||\n");
	printf( YELLOW " ========================================================\n");
	printf( NORMAL "\n Pressione <enter> para voltar\n");
	getchar();
}

void play() {
	int i, j;
	Rank player;
	srand(time(NULL));

	if(ranked)
	{
		int registered=1;
		do
		{
			if(!registered) printf( RED BOLD " !No maximo 10 caracteres!\n");
			printf( NORMAL " Informe seu nick, de ate 10 caracteres: " YELLOW BOLD );
			scanf("%[^\n]s", player.name);
			printf(""NORMAL);
			getchar();
			if(strlen(player.name)>10) registered=0;
			else registered=1;
		} while(!registered);
	}

	murdered=0;
	cornered=0;
	starved=0;
	tQ_range=((tQ_base)*3/4);

	score=0;
    energy=100;
	playing=1;
	init_board();
	do {
		if(bc[0][trB-1][0]>=0) for(i=0, j=trB-1; i<qtB; i++) /* ja remove a ultima instancia do B para que sua coordenada seja ocupavel */
			board[by][bx]='.';
		input();
		if(!playing) break;
		update_board();
		render_board();
	} while(playing);
	game_over();

	if(ranked) update_ranking(player);
}

void init_board() {
	int i, j;

	x=wdt/2;
	y=hgt/2;
	turn=1;

	for (i=0; i<hgt; i++) for (j=0; j<wdt; j++) board[i][j]='.'; /* inicializa a matriz tabuleiro */

	board[y][x]='C';

	init_O(); /* inicializa os O's */

	init_B(); /* inicializa os B's */

	init_X(); /* inicializa os X's */

	init_Q(); /* inicializa os Q's */

	render_board();
}

void init_O() {
	int i;

	for(i=0; i<qtO; i++)
	{
		reset(&oy, &ox); /* da uma coordenada para o O(i) */
		board[oy][ox]='O';

		ctO[i]=-1; /* o valor negativo indica que O(i) esta presente no tabuleiro */
	}
}

void init_B() {
	int i, j;

	for(i=0, j=0; i<qtB; i++, j=0)
	{
		reset(&by, &bx); /* da uma coordenada para o B(i,0) */
		board[by][bx]='b'; /* posiciona os B(i,0)'s */

		for(j=1; j<trB; j++) /* as demais instancias recebem um nmr negativo */
		{
			by=-1;
			bx=-1;
		}
	}
}

void init_X() {
	int i;

	if(difficulty>=1) X_cross_screen=1;
	if(difficulty>=2) X_improved_percep=1;
	if(difficulty>=3) X_eats_diag=1;
	if(difficulty>=4) X_warmonger=1;

	for(i=0; i<qtX; i++)
	{
		reset(&xy, &xx); /* da uma coordenada para o X(i) */
		board[xy][xx]='y'; /* posiciona os X(i)'s */

		if(bhX>0) ctX[i]=bhX; /* os contadores sao iniciados */
		else X_warmonger=1;

		cdX[i]=0; /* representa que esse X esta vivo */
	}
}

void init_Q() {
	int i;
	for(i=0; i<qtQ; i++)
	{
		qy=-1; /* recebe um valor negativo pois esta fora do tabuleiro */
		qx=-1;
		ctQ[i]=rand()%(2*cdQ)+0.5*cdQ;
	}
}

void update_board() {
	int i, j, diff;

	// for(i=0; i<hgt; i++) for(j=0; j<wdt; j++) board[i][j]='.';

	board[y][x]='C'; /* posiciona o C */

	check_O(); /* checa se um O foi coletado */

	mov_B(); /* reposiciona os B's */

	mov_X(); /* posiciona os X's */

	update_Q(); /* posiciona ou explode os Q's */

	/* checa se restam movimentos */
	turn++;
	if(moves==0)
	{
		playing=0;
		starved=1;
		board[y][x]='c';
	}
}

void check_O() {
	int i;

	for(i=0; i<qtO; i++)
	{
		if((oy>=0)&&(board[oy][ox]==board[y][x]))
		{
			oy=-1;
			ox=-1;
			board[y][x]='V';
			score+=O_value;
			energy+=O_energy;
			ctO[i]=rand()%(qtO)+3;
			DEBUG_COLLECT;
		}
		else
		{
			ctO[i]-=1;
			if(ctO[i]==0)
			{
				ctO[i]=-1;
				reset(&oy, &ox);
				board[oy][ox]='O';
			}
		}
	}
}

void update_Q() {
	int i;

	clear_Q();

	for(i=0; i<qtQ; i++)
	{
		if(ctQ[i]>0) /* bomba esta recarregando */
		{			
			ctQ[i]-=1;
			if(ctQ[i]==0)
			{
				reset(&qy, &qx);
				board[qy][qx]='Q';
				ctQ[i]=ger_timer;
			}
		}
		else /* ctQ[i]<0 */ /* bomba vai explodir */
		{
			ctQ[i]+=1;
			if(ctQ[i]==0)
			{
				blast_Q(i);
				ctQ[i]=ger_cd;
			}
		}
	}
}

void clear_Q() {
	int i, j;
	for(i=0; i<hgt; i++) for(j=0; j<wdt; j++) if((board[i][j]=='#')||(board[i][j]=='!')) board[i][j]='.';
}

void blast_Q(int i) {
	int j;
	int aux=i/* armazena o identificador do Q */, aux2/* armazena o j */;
	int halt_up=0, halt_left=0, halt_down=0, halt_right=0; /* se ativos, impedem o sentido impedido de continuar se alastrando */
	int warp_up=0, warp_left=0, warp_down=0, warp_right=0; /* somado a uma coordenada de teste se ela for testar do outro lado da tela */
	board[qy][qx]='!';

	for(j=0; j<rdQ; j++)
	{
		if(!halt_up) /* explode pra cima */
		{
            if(qy-j+warp_up==0) warp_up+=hgt;
			if(empty_above(qy-j+warp_up, qx)) board[qy-j-1+warp_up][qx]='#';
			else if(char_above('C', qy-j+warp_up, qx)) /* mata o C */
			{
				playing=0;
				blownup=1;
				board[y][x]='$';
			}
			else if(char_above('X', qy-j+warp_up, qx)) /* mata o X */
			{
				i=find_ch('X', qy-j-1+warp_up, qx); /* i se torna identificador do X morto */
				cdX[i]=rand()%(tcdX)+tcdX;
				board[xy][xx]='#';
				xy=-1;
				xx=-1;
				i=aux;
			}
			else if(char_above('b', qy-j+warp_up, qx)) /* mata a cabeca do B */
			{
				aux2=j;
				i=find_ch('B', qy-j-1+warp_up, qx); /* i se torna identificador do B morto */
				j=0;
				board[by][bx]='#';
				reset(&by, &bx);
				i=aux;
				j=aux2;
			}
			else if(char_above('O', qy-j+warp_up, qx)) /* destroi o O */
			{
				i=find_ch('O', qy-j-1+warp_up, qx);
				board[oy][ox]='#';
				oy=-1;
				ox=-1;
				ctO[i]=rand()%(qtO)+3;
				i=aux;
			}
			else if(char_above('Q', qy-j+warp_up, qx)) /* detona a outra bomba */
			{
				i=find_ch('Q', qy-j-1+warp_up, qx);
				blast_Q(i);
				ctQ[i]=ger_cd;
				i=aux;
			}
			else if(char_above('#', qy-j+warp_up, qx)){} /* ignora o fogo */
			else
			{
				halt_up=1;
			}
		}
		if(!halt_left) /* explode pra esquerda */
		{
            if(qx-j+warp_left==0) warp_left+=wdt;
			if(empty_left(qy, qx-j+warp_left)) board[qy][qx-j-1+warp_left]='#';
			else if(char_left('C', qy, qx-j+warp_left)) /* mata o C */
			{
				playing=0;
				blownup=1;
				board[y][x]='$';
			}
			else if(char_left('X', qy, qx-j+warp_left)) /* mata o X */
			{
				i=find_ch('X', qy, qx-j-1+warp_left); /* i se torna identificador do X morto */
				cdX[i]=rand()%(tcdX)+tcdX;
				board[xy][xx]='#';
				xy=-1;
				xx=-1;
				i=aux;
			}
			else if(char_left('b', qy, qx-j+warp_left)) /* mata a cabeca do B */
			{
				aux2=j;
				i=find_ch('B', qy, qx-j-1+warp_left); /* i se torna identificador do B morto */
				j=0;
				board[by][bx]='#';
				reset(&by, &bx);
				i=aux;
				j=aux2;
			}
			else if(char_left('O', qy, qx-j+warp_left)) /* destroi o O */
			{
				i=find_ch('O', qy, qx-j-1+warp_left);
				board[oy][ox]='#';
				oy=-1;
				ox=-1;
				ctO[i]=rand()%(qtO)+3;
				i=aux;
			}
			else if(char_left('Q', qy, qx-j+warp_left)) /* detona a outra bomba */
			{
				i=find_ch('Q', qy, qx-j-1+warp_left);
				blast_Q(i);
				ctQ[i]=ger_cd;
				i=aux;
			}
			else if(char_left('#', qy, qx-j+warp_left)){} /* ignora o fogo */
			else
			{
				halt_left=1;
			}
		}
		if(!halt_down) /* explode pra baixo */
		{
            if(qy+j-warp_down==hgt-1) warp_down+=hgt;
			if(empty_below(qy+j-warp_down, qx)) board[qy+j+1-warp_down][qx]='#';
			else if(char_below('C', qy+j-warp_down, qx)) /* mata o C */
			{
				playing=0;
				blownup=1;
				board[y][x]='$';
			}
			else if(char_below('X', qy+j-warp_down, qx)) /* mata o X */
			{
				i=find_ch('X', qy+j+1-warp_down, qx); /* i se torna identificador do X morto */
				cdX[i]=rand()%(tcdX)+tcdX;
				board[xy][xx]='#';
				xy=-1;
				xx=-1;
				i=aux;
			}
			else if(char_below('b', qy+j-warp_down, qx)) /* mata a cabeca do B */
			{
				aux2=j;
				i=find_ch('B', qy+j+1-warp_down, qx); /* i se torna identificador do B morto */
				j=0;
				board[by][bx]='#';
				reset(&by, &bx);
				i=aux;
				j=aux2;
			}
			else if(char_below('O', qy+j-warp_down, qx)) /* destroi o O */
			{
				i=find_ch('O', qy+j+1-warp_down, qx);
				board[oy][ox]='#';
				oy=-1;
				ox=-1;
				ctO[i]=rand()%(qtO)+3;
				i=aux;
			}
			else if(char_below('Q', qy+j-warp_down, qx)) /* detona a outra bomba */
			{
				i=find_ch('Q', qy+j+1-warp_down, qx);
				blast_Q(i);
				ctQ[i]=ger_cd;
				i=aux;
			}
			else if(char_below('#', qy+j-warp_down, qx)){} /* ignora o fogo */
			else
			{
				halt_down=1;
			}
		}
		if(!halt_right) /* explode pra direita */
		{
            if(qx+j-warp_right==wdt-1) warp_right+=wdt;
			if(empty_right(qy, qx+j-warp_right)) board[qy][qx+j+1-warp_right]='#';
			else if(char_right('C', qy, qx+j-warp_right)) /* mata o C */
			{
				playing=0;
				blownup=1;
				board[y][x]='$';
			}
			else if(char_right('X', qy, qx+j-warp_right)) /* mata o X */
			{
				i=find_ch('X', qy, qx+j+1-warp_right); /* i se torna identificador do X morto */
				cdX[i]=rand()%(tcdX)+tcdX;
				board[xy][xx]='#';
				xy=-1;
				xx=-1;
				i=aux;
			}
			else if(char_right('b', qy, qx+j-warp_right)) /* mata a cabeca do B */
			{
				aux2=j;
				i=find_ch('B', qy, qx+j+1-warp_right); /* i se torna identificador do B morto */
				j=0;
				board[by][bx]='#';
				reset(&by, &bx);
				i=aux;
				j=aux2;
			}
			else if(char_right('O', qy, qx+j-warp_right)) /* destroi o O */
			{
				i=find_ch('O', qy, qx+j+1);
				board[oy][ox]='#';
				oy=-1;
				ox=-1;
				ctO[i]=rand()%(qtO)+3;
				i=aux;
			}
			else if(char_right('Q', qy, qx+j-warp_right)) /* detona a outra bomba */
			{
				i=find_ch('Q', qy, qx+j+1-warp_right);
				blast_Q(i);
				ctQ[i]=ger_cd;
				i=aux;
			}
			else if(char_right('#', qy, qx+j-warp_right)){} /* ignora o fogo */
			else
			{
				halt_right=1;
			}
		}
	}
	qy=-1;
	qx=-1;
}

int find_ch(char ch, int cy, int cx) {
	int i, j=0;

	switch(ch)
	{
		case 'X': for(i=0; i<qtX; i++) if((cy==xy)&&(cx==xx)) return i;

		case 'O': for(i=0; i<qtO; i++) if((cy==oy)&&(cx==ox)) return i;

		case 'Q': for(i=0; i<qtQ; i++) if((cy==qy)&&(cx==qx)) return i;

		case 'B': for(i=0; i<qtB; i++) if((cy==by)&&(cx==bx)) return i;

		default: return -1;
	}
}

void mov_B() {
	int dr, i, j;

	/* desloca as instancias para baixo (menos a inst 0) */
	for(i=0; i<qtB; i++) for(j=0; j<trB-1; j++)
	{
		bc[i][trB-1-j][0]=bc[i][trB-2-j][0];
		bc[i][trB-1-j][1]=bc[i][trB-2-j][1];
	}

	for(i=0, j=0; i<qtB; i++, j=0)
	{
		rand_mov(&by, &bx); /* j=0 permite a utilizacao do macro by/bx, o rand_mov move a instancia 0 aleatoriamente */
		board[by][bx]='b'; /* o renderizador reconhece que esse B tem outra cor */
		for (j=1; j<trB; j++) if(by>=0) board[by][bx]='B'; /* coloca as demais instancias, menos as inexistentes(<0) */		
	}
}

void mov_X() {
	int i;

	for(i=0; i<qtX; i++)
	{
		if(cdX[i]==0)
		{
			if(!X_warmonger)
			{
				if(ctX[i]>0) /* X age pacificamente */
				{
					ctX[i]-=1; /* desconta o contador */
					rand_mov(&xy, &xx);
					board[xy][xx]='y';
					if(ctX[i]==0) /* testa se o contador ainda nao se esgotou */
					{
						ctX[i]=-(rand()%(bhX)+bhX+5); /* ctX[i] recebe um valor negativo que vai de bhX ate 2*bhX */
					}
				}
				else
				{
					ctX[i]+=1; /* aumenta o contador */
					chase_C(i);
					if(ctX[i]==0) /* testa se o contador ainda nao se esgotou */
					{
						ctX[i]=bhX; /* o contador volta ao estado bhX */
					}
				}
			}
			else chase_C(i);
		}
		else
		{
			cdX[i]-=1;
			if(cdX[i]==0) reset(&xy, &xx);
		}
	}
}

void chase_C(int i) {
	dist dt=calc_d(i);
	int order=rand()%(2);
	int ate_C=0; /* determina como o X vai ser atualizado */

	// DEBUG_D;

	if(eat_range(dt)) /* fim de jogo por assassinato */
	{
		ate_C=1;
		playing=0;
		murdered=1;
	}

	// nos casos de mesma ou adjacente linha/coluna, a variavel order altera a ordem em que os 2 comandos sao processados
		else if(((mag(dt.dy)==1)||(mag(dt.daby)==1)||(dt.dy==0))&&order) /* o X(i) anda horizontalmente para nao ficar indo pra cima e pra baixo */
		{
			if((mag(dt.dx)<=mag(dt.dabx))||(!X_cross_screen))
			{
				dt.dx>0? mov_s(3, i, dt) : mov_s(1, i, dt);
			}
			else dt.dabx>0? mov_s(3, i, dt) : mov_s(1, i, dt);
		}

		else if(((mag(dt.dx)==1)||(mag(dt.dabx)==1)||(dt.dx==0))&&order) /* o X(i) anda verticalmente para nao ficar indo de um lado pro outro */
		{
			if((mag(dt.dy)<=mag(dt.daby))||(!X_cross_screen)) dt.dy>0? mov_s(2, i, dt) : mov_s(0, i, dt);
			else dt.daby>0? mov_s(2, i, dt) : mov_s(0, i, dt);
		}

		else if(((mag(dt.dx)==1)||(mag(dt.dabx)==1)||(dt.dx==0))&&!order) /* o X(i) anda verticalmente para nao ficar indo de um lado pro outro */
		{
			if((mag(dt.dy)<=mag(dt.daby))||(!X_cross_screen)) dt.dy>0? mov_s(2, i, dt) : mov_s(0, i, dt);
			else dt.daby>0? mov_s(2, i, dt) : mov_s(0, i, dt);
		}

		else if(((mag(dt.dy)==1)||(mag(dt.daby)==1)||(dt.dy==0))&&!order) /* o X(i) anda horizontalmente para nao ficar indo pra cima e pra baixo */
		{
			if((mag(dt.dx)<=mag(dt.dabx))||(!X_cross_screen)) dt.dx>0? mov_s(3, i, dt) : mov_s(1, i, dt);
			else dt.dabx>0? mov_s(3, i, dt) : mov_s(1, i, dt);
		}
	// 

	else if(X_better_cross) switch(lowest4_uns(dt.dy, dt.dx, dt.daby, dt.dabx)) /* movimenta atraves bordas */
	{
		case 0: dt.dy<0? mov_s(0, i, dt) : mov_s(2, i, dt); break;
		case 1: dt.dx<0? mov_s(1, i, dt) : mov_s(3, i, dt); break;
		case 2: dt.daby<0? mov_s(0, i, dt) : mov_s(2, i, dt); break;
		case 3: dt.dabx<0? mov_s(1, i, dt) : mov_s(3, i, dt);
	}

	else if(mag(dt.dy)<=mag(dt.dx)) dt.dy<0? mov_s(0, i, dt) : mov_s(2, i, dt); /* movimento burro */
	else dt.dx<0? mov_s(1, i, dt) : mov_s(3, i, dt);

	if(!ate_C) board[xy][xx]='X'; /* checa como atualizar o X */
	else
	{
		board[xy][xx]='.';
		board[y][x]='x';
	}
}

dist calc_d(int i) {
	dist dt;
	dt.dy=y-xy;
	dt.dx=x-xx;
	if(xy<=hgt/2) dt.daby=-(xy+(hgt-y));
	else dt.daby=((hgt-xy)+y);
	if(xx<=wdt/2) dt.dabx=-(xx+(wdt-x));
	else dt.dabx=((wdt-xx)+x);
	return dt;
}

void mov_s(int dr, int i, dist dt) {
	if(!X_improved_percep) mov(dr, &xy, &xx); /* move o X normalmente */
	else /* testa por onde ele pode ir */
	{
		switch(dr)
		{
			case 0:
				if(empty_above(xy, xx)) mov(0, &xy, &xx);
				else outline(0, i, dt, 0, 0);
			break;

			case 1:
				if(empty_left(xy, xx)) mov(1, &xy, &xx);
				else outline(1, i, dt, 0, 0);
			break;

			case 2:
				if(empty_below(xy, xx)) mov(2, &xy, &xx);
				else outline(2, i, dt, 0, 0);
			break;

			case 3:
				if(empty_right(xy, xx)) mov(3, &xy, &xx);
				else outline(3, i, dt, 0, 0);
			break;
		}
	}
}

void outline(int idr, int i, dist dt, int alt, int n) {
	if(n==3) rand_mov(&xy, &xx);
	int j, k, l, m; /* iteradoes */
	int cy=xy, cx=xx; /* coordenatas temporarias */
	int adt; /* distancia perpendicular a direcao intencionada */
	int finished=0;
	int hinder_left=0, hinder_right=0, hinder_above=0, hinder_below=0; /* impedem o X de testar movimentos para um lado bloqueado */

	if((idr==0)||(idr==2))
	{
		if(X_cross_screen) adt=rt_lowest_mag(dt.dx, dt.dabx);
		else adt=dt.dx;
	}
	else if(X_cross_screen) adt=rt_lowest_mag(dt.dy, dt.daby);
	else adt=dt.dy;

	if(alt) adt*=-1; /* o modificador altera o sentido da distancia */

	if(adt>=0) /* determina se o X anda positivamente ou negativamente no eixo */
	{
		switch(idr) /* determina a direcao do movimento */
		{
			case 0:
				for(j=0; j<adt; j++) /* procura brechas entre X e C diretamente */
				{
					if(empty_right(cy, cx))
					{
						cx++;
						if(empty_above(cy, cx))
						{
							mov(3, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(3, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(3, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j<0)&&(!hinder_left)) /* checa pela esquerda */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_left(cy, cx))
							{
								cx--;
								if(empty_above(cy, cx))
								{
									mov(1, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_left=1; /* so vai checar pela direita agora */
								break;
							}
						} j=-j+1;
					}
					else /* j>0 */
					{
						cx=(xx+j-1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_right(cy, cx))
							{
								cx++;
								if(empty_above(cy, cx))
								{
									mov(3, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(3, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_left) j=-j-1;
						else j+=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 1:
				for(j=0; j<adt; j++) /* procura brechas entre X e C diretamente */
				{
					if(empty_below(cy, cx))
					{
						cy++;
						if(empty_left(cy, cx))
						{
							mov(2, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(2, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(2, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j<0)&&(!hinder_above)) /* checa por cima */
					{
						cy=(xy+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_above(cy, cx))
							{
								cy--;
								if(empty_left(cy, cx))
								{
									mov(0, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_above=1; /* so vai checar por baixo agora */
								break;
							}
						} j=-j+1;
					}
					else /* j>0 */
					{
						cy=(xy+j-1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_below(cy, cx))
							{
								cy++;
								if(empty_left(cy, cx))
								{
									mov(2, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(2, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_above) j=-j-1;
						else j+=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 2:
				for(j=0; j<adt; j++) /* procura brechas entre X e C diretamente */
				{
					if(empty_right(cy, cx))
					{
						cx++;
						if(empty_below(cy, cx))
						{
							mov(3, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(3, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(3, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j<0)&&(!hinder_left)) /* checa pela esquerda */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_left(cy, cx))
							{
								cx--;
								if(empty_below(cy, cx))
								{
									mov(1, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_left=1; /* so vai checar pela direita agora */
								break;
							}
						} j=-j+1;
					}
					else /* j>0 */
					{
						cx=(xx+j-1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_right(cy, cx))
							{
								cx++;
								if(empty_below(cy, cx))
								{
									mov(3, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(3, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_left) j=-j-1;
						else j+=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 3:
				for(j=0; j<adt; j++) /* procura brechas entre X e C diretamente */
				{
					if(empty_below(cy, cx))
					{
						cy++;
						if(empty_right(cy, cx))
						{
							mov(2, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(2, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(2, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j<0)&&(!hinder_above)) /* checa por cima */
					{
						cy=(xy+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_above(cy, cx))
							{
								cy--;
								if(empty_right(cy, cx))
								{
									mov(0, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_above=1; /* so vai checar por baixo agora */
								break;
							}
						} j=-j+1;
					}
					else /* j>0 */
					{
						cy=(xy+j-1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_below(cy, cx))
							{
								cy++;
								if(empty_right(cy, cx))
								{
									mov(2, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(2, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_above) j=-j-1;
						else j+=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;
		}
	}
	else
	{
		switch(idr) /* determina a direcao do movimento */
		{
			case 0:
				for(j=0; j>adt; j--) /* procura brechas entre X e C diretamente */
				{
					if(empty_left(cy, cx))
					{
						cx--;
						if(empty_above(cy, cx))
						{
							mov(1, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(1, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(1, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j>0)&&(!hinder_right)) /* checa pela direita */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_right(cy, cx))
							{
								cx++;
								if(empty_above(cy, cx))
								{
									mov(3, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_right=1; /* so vai checar pela esquerda agora */
								break;
							}
						} j=-j-1;
					}
					else /* j<0 */
					{
						cx=(xx+j+1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_left(cy, cx))
							{
								cx--;
								if(empty_above(cy, cx))
								{
									mov(1, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(1, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_right) j=-j+1;
						else j-=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 1:
				for(j=0; j>adt; j--) /* procura brechas entre X e C diretamente */
				{
					if(empty_above(cy, cx))
					{
						cy--;
						if(empty_left(cy, cx))
						{
							mov(0, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(0, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(0, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j>0)&&(!hinder_below)) /* checa pela direita */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_below(cy, cx))
							{
								cy++;
								if(empty_left(cy, cx))
								{
									mov(2, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_below=1; /* so vai checar pela esquerda agora */
								break;
							}
						} j=-j-1;
					}
					else /* j<0 */
					{
						cx=(xx+j+1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_above(cy, cx))
							{
								cy--;
								if(empty_left(cy, cx))
								{
									mov(0, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(0, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_below) j=-j+1;
						else j-=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 2:
				for(j=0; j>adt; j--) /* procura brechas entre X e C diretamente */
				{
					if(empty_left(cy, cx))
					{
						cx--;
						if(empty_below(cy, cx))
						{
							mov(1, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(1, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(1, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j>0)&&(!hinder_right)) /* checa pela direita */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_right(cy, cx))
							{
								cx++;
								if(empty_below(cy, cx))
								{
									mov(3, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_right=1; /* so vai checar pela esquerda agora */
								break;
							}
						} j=-j-1;
					}
					else /* j<0 */
					{
						cx=(xx+j+1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_left(cy, cx))
							{
								cx--;
								if(empty_below(cy, cx))
								{
									mov(1, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(1, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_right) j=-j+1;
						else j-=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;

			case 3:

				for(j=0; j>adt; j--) /* procura brechas entre X e C diretamente */
				{
					if(empty_above(cy, cx))
					{
						cy--;
						if(empty_right(cy, cx))
						{
							mov(0, &xy, &xx);
							finished=1; break;
						}
					}
					else if(j==0)
					{
						outline(0, i, dt, 1, n+1);
						finished=1; break;
					}
					else
					{
						mov(0, &xy, &xx);
						finished=1; break;
					}
				} j*=-1; /* para que ele comece a buscar pelo outro lado agora */

				while(!finished) /* vai testando por um sentido e depois por outro, alternadamente */
				{
					if((j>0)&&(!hinder_below)) /* checa pela direita */
					{
						cx=(xx+j+adt); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* checa adt/2 vezes */		
						{
							if(empty_below(cy, cx))
							{
								cy++;
								if(empty_right(cy, cx))
								{
									mov(2, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								hinder_below=1; /* so vai checar pela esquerda agora */
								break;
							}
						} j=-j-1;
					}
					else /* j<0 */
					{
						cx=(xx+j+1); /* posiciona o testador */
						for(k=1; k<=(adt/2); k++) /* testa adt/2 vezes */
						{
							if(empty_above(cy, cx))
							{
								cy--;
								if(empty_right(cy, cx))
								{
									mov(0, &xy, &xx);
									finished=1; break;				
								}
							}
							else
							{
								mov(0, &xy, &xx); /* como esse sentido esta livre o X vai nele */
								finished=1;
								break;
							}
						}
						if(!hinder_below) j=-j+1;
						else j-=2;
					}

					if((!finished)&&(mag(j)>=40))
					{
						rand_mov(&xy, &xx); /* *flips table* */
						finished=1;
					}
				}
			break;
		}
	}
}

int lowest4_uns(int n0, int n1, int n2, int n3) {
	int t0, t1, call; /* temporary */
	n0=mag(n0);
	n1=mag(n1);
	n2=mag(n2);
	n3=mag(n3);

	if(n0<n1) t0=n0;
	else t0=n1;
	if(n2<n3) t1=n2;
	else t1=n3;
	if(t0<t1) call=0;
	else call=1;

	if(!call)
	{
		if(t0==n0) return 0;
		else return 1;
	}
	else
	{
		if(t1==n2) return 2;
		else return 3;
	}
}

void reset(int* cy, int* cx) {
	int ny, nx;

	do	{
		ny=rand()%(hgt); /* atribuimos as variaveis valores validos de coordenada */
		nx=rand()%(wdt);
	} while ( (board[ny][nx]!='.')||char_around('C', ny, nx)||char_around('V', ny, nx) );

	*cy=ny;
	*cx=nx;

	if(turn!=1)	DEBUG_RESET; /* printa na tela um aviso em RED que a funcao foi chamada */
}

void input() {
	char c[100];
	int dr;

	/* checa se o C esta encurralado */
	if(kinky(y, x))
	{ /* fim de jogo por encurralamento */
		playing=0;
		cornered=1;
	}

	if(playing)
	{
		printf(" ");
		scanf("%[^\n]s", c); /* le o comando em string para nao baguncar o buffer de teclado */
		getchar();
		switch (c[0])
		{
			case 'w':
				if(char_above('O', y, x))
				{
					board[y][x]='.';
					y--;
					warp_b(&y, &x); /* checa se e necessario teleportar atraves das bordas da tela */
				}
				else mov(0, &y, &x); 
			break;

			case 'a':
				if(char_left('O', y, x))
				{
					board[y][x]='.';
					x--;
					warp_b(&y, &x); /* checa se e necessario teleportar atraves das bordas da tela */
				}
				else mov(1, &y, &x); 
			break;

			case 's':
				if(char_below('O', y, x))
				{
					board[y][x]='.';
					y++;
					warp_b(&y, &x); /* checa se e necessario teleportar atraves das bordas da tela */
				}
				else mov(2, &y, &x); 
			break;

			case 'd':
				if(char_right('O', y, x))
				{
					board[y][x]='.';
					x++;
					warp_b(&y, &x); /* checa se e necessario teleportar atraves das bordas da tela */
				}
				else mov(3, &y, &x); 
			break;

			case '0': playing=0; break;
			default: rand_mov(&y, &x);
		}
	}
}

void mov(int dr, int* cy, int* cx) {
	board[*cy][*cx]='.';
	int stuck;

	/* checa se a unidade esta encuralada, se estiver reseta sua posicao */
	if(kinky(*cy, *cx)) reset(cy, cx);

	do
	{
		stuck=0;
		switch (dr)
		{ /* checa se a posicao esta disponivel ou se ela vai atravessar a extremidade da tela e esta disponivel ai */
			case 0: empty_above(*cy, *cx) ?
				*cy-=1:(stuck+=1); break;
			case 1: empty_left(*cy, *cx) ?
				*cx-=1:(stuck+=1); break;
			case 2: empty_below(*cy, *cx) ?
				*cy+=1:(stuck+=1); break;
			case 3: empty_right(*cy, *cx) ?
				*cx+=1:(stuck+=1); break;
		}	
		dr=rand()%(5);
	} while(stuck);
	warp_b(cy, cx);
}

void warp_b(int* cy, int* cx) {
	if(*cy<0) *cy=hgt-1;
	if(*cy>=hgt) *cy=0;
	if(*cx<0) *cx=wdt-1;
	if(*cx>=wdt) *cx=0;
}

void render_board() {
	int i, j;

	CLEAR;
	printf("\n\n");
	printf(" Pontos: %d			Energia: %d\n\n", mag(score), moves);
	DEBUG_QT;

	/* implementar um fim para moves */

	// gerando o tabuleiro:
	/*primeira linha*/
	printf( YELLOW "  ");
	for(i=0; i<wdt*2+3; i++)
		printf("_");
	printf("  \n");

	/*segunda linha*/
	printf(" /\\");
	for(i=0; i<(wdt*2)+2; i++)
		printf(" ");
	printf("\\ \n");

	/*terceira linha*/
	printf(" \\_|");
	for(i=0; i<wdt*2+1; i++)
		printf(" ");
	printf("| \n");

	/*parte bruta*/
	for (i=0; i<hgt; i++)
	{
		printf("   |");
		for (j=0; j<wdt; j++)
		{
			switch (board[i][j])
			{
			case 'C': printf( YELLOW BOLD " C" NORMAL ); break;
			case 'c': printf( RED BOLD " c" NORMAL ); break;
			case 'V': printf( GREEN " C" NORMAL ); break;
			case 'O': printf( CIAN " O" NORMAL ); break;
			case 'B': printf( NORMAL BOLD " B" NORMAL ); break;
			case 'b': printf( YELLOW " B" NORMAL ); break;
			case 'X': printf( MAGENTA BOLD " X" NORMAL ); break;
			case 'x': printf( RED BOLD " X" NORMAL ); break;
			case 'y': printf( MAGENTA " X" NORMAL ); break;
			case 'Q': printf( CIAN BOLD " Q" NORMAL ); break;
			case '!': printf( RED BOLD " !" NORMAL ); break;
			case '#': printf( RED " #" NORMAL ); break;
			case '$': printf( RED BOLD " #" NORMAL ); break;
			case 'Z': printf( NORMAL BOLD " Z" NORMAL ); break;
			case '.': printf( BLACK BOLD " ." NORMAL ); break;
			default: printf(" ?");
			}
		}
		printf( YELLOW " | \n");
	}

	/*antepenultima linha*/
	printf("   |");
	for (i=0; i<wdt*2+1; i++)
		printf(" ");
	printf("| \n");

	/*penultima linha*/
	printf("   |  ");
	for (i=0; i<wdt*2-1; i++)
		printf("_");
	printf("|_\n");

	/*ultima linha*/
	printf("   \\_/");
	for (i=0; i<wdt*2-1; i++)
		printf("_");
	printf("_/\n" NORMAL );
	DEBUG_XY;
}

void debug_set_X(int i, int cy, int cx) {
	xy=cy;
	xx=cx;
	board[xy][xx]='X';
}

void debug_set_Z(int cy, int cx) {
	board[cy][cx]='Z';
}