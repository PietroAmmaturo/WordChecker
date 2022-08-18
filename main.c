#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPH_LEN 64
#define TEST_LEN 6
#define EOW ' '
#define EOI '0'
#define EOS '\0'
#define EOL '\n'
#define MAX_COMMAND_LENGTH 32
#define START 'A'

// To force compiler to use 1 byte packaging
#pragma pack(1)

// callgrind_annotate --tree=calling callgrind.out.7096 > out
//  tipi
//   struttura dati contenente le informazioni relative ai vari inserimanti
typedef struct info_s
{
	// 0 contenente il numero minimo di occorrenze nella parola (sia incognite che fissate)
	// pari a -1 se si è scoperto che la lettera non è presente..
	//'A' è in posizione 0, 'B' in posizione 1, eccetera
	char discoveredOccurrences[ALPH_LEN];
	_Bool isDiscoveredOccurrencesUTD;
	// occorrenze delle lettere nella parola
	char trueOccurrences[ALPH_LEN];

	// indica per ogni lettera se il numero di occorrenze è il numero definitivo.
	// Il numero di occorrenze è definitivo se ho testato che una parola ha una lettera "sbagliata"
	// dopo aver scoperto che la stessa lettera era "giusta al posto sbagliato" o "giusta al posto giusto" in una posizione precedente
	// o se il contatore di una lettera è maggiore del numero di lettere effettive
	_Bool isDefinitive[ALPH_LEN];

	//(contenente la lista di possibili caratteri che si possono trovare in quella posizione, organizzato come un array di bool.
	// NB: se un carattere è fissato in una posizione (giusto al posto giusto), sarà presente solo lui,
	// se invece un carattere in quella posizione è risultato "giusto al posto sbagliato", sicuramente non sarà presente in quella posizione)
	_Bool (*isPositionOfCharacterValid)[ALPH_LEN];
	_Bool isIsPositionOfCharacterValidUTD;

	// numero di lettere che appaiono almeno una volta
	unsigned int numberOfDiscoveredOccurrences : 4;

	// profondità
	unsigned int maxDepth : 4;

	// lunghezza della parola
	unsigned int length : 4;

	// conta le lettere che hanno abbaztanza occorrenze nela parola mentre si filtra il dizionario
	unsigned int lettersWithEnoughOccurrences : 4;

	unsigned int matchId : 8;

	// conta il numero di parole rimanenti nel dizionario al termine del filtraggio
	unsigned int filteredCounter;

	// contatore di caratteri utile sia quando si filtra che quando si confrontano le parole
	unsigned int characterCounter[64];

	// puntatore ad int, usato per memorizzare la parola in diverse situazioni
	int *word;
	// puntatore ad int, usato per memorizzare la soluzione in una partita
	int *solution;
	// puntatore ad int, usato per memorizzare il risultato del confronto
	char *result;
} info_t;

// come strutturo un nodo dell'albero rappresentante il dizionario
typedef struct node_s
{
	unsigned int killerId : 8;
	// carattere contenuto nel nodo
	unsigned int index : 6;
} node_t;

// come strutturo un nodo dell'albero rappresentante il dizionario
typedef struct twig_s
{
	node_t node;
	// profondità
	int depth : 4;
	// numero di figli
	unsigned int maxChildIndex : 6;
	// puntatore all'array di figli
	void *children;
} twig_t;

// dichiarazioni

int characterToIndex(char c);

char indexToCharacter(int n);

int findChild(void *h, int c);

int findLeaf(void *h, int c);

_Bool findWord(info_t *info, void *head);

void *addFirstNode(void *h);

int findOrAddNode(void *h, int c);

void findOrAddLeaf(void *h, int c);

void addWord(info_t *info, void *head);

void addWords(info_t *info, void *head);

void printDictionary(info_t *info, void *h);

void printNode(info_t *info, void *h);

void printLeaf(info_t *info, void *h);

void filterLeaf(info_t *info, void *h);

void filterNode(info_t *info, void *h);

void filterDictionary(info_t *info, void *h);

void comments();
/*
void filterPositions(info_t *info, branched_twig_t *h);

void filterPositionsIntwig(info_t *info, branched_twig_t *h);

void filterOccurrences(info_t *info, branched_twig_t *h);

void filterOccurrencesIntwig(info_t *info, branched_twig_t *h);
*/
void startMatch(info_t *info, void *head, _Bool *isFree);

void compareWords(info_t *info, _Bool *isFree);

//////////////
//	MAYBE	//
//////////////
void comments()
{
	/*
	void filterPositions(info_t *info, branched_twig_t *h)
	{
		// filtro il dizionario aggiornando i validi
		info->depth = -1;
		// inizializzo il contatore delle filtrate a zero
		info->filteredCounter = 0;

		filterPositionsIntwig(info, h->child);

		// è tutto uptodate
		info->isIsPositionOfCharacterValidUTD = 1;
	}
	void filterPositionsIntwig(info_t *info, branched_twig_t *h)
	{
		if (h->valid)
		{
			// aumento la profondità
			info->depth++;
			// se il carattere non si può trovare a quella posizione (profondità)
			if (!(info->isPositionOfCharacterValid[info->depth][h->index]))
			{
				// rendo non valido il nodo
				h->valid = 0;
			}
			// quando raggiungo la fine della parola
			if (h->child == NULL)
			{

				// conto la filtrata
				info->filteredCounter++;
			}
			// child
			filterPositionsIntwig(info, h->child);
			;

			// al ritorno decremento la profondità
			info->depth--;
		}
		// sibiling
		if (h->sibiling)
			filterPositionsIntwig(info, h->sibiling);
	}

	void filterOccurrences(info_t *info, branched_twig_t *h)
	{
		int i;
		// inizializzo il contatore di caratteri a zero
		for (i = 0; i < ALPH_LEN; i++)
		{
			info->characterCounter[i] = 0;
		}
		// inizializzo il numero di lettere che hanno abbastanza occorrenze
		info->lettersWithEnoughOccurrences = 0;
		// filtro il dizionario aggiornando i validi
		info->depth = -1;
		// inizializzo il contatore delle filtrate a zero
		info->filteredCounter = 0;

		filterOccurrencesIntwig(info, h->child);

		// è tutto uptodate
		info->isDiscoveredOccurrencesUTD = 1;
	}
	void filterOccurrencesIntwig(info_t *info, branched_twig_t *h)
	{
		if (h->child)
		{
			// conto i caratteri
			info->characterCounter[h->index]++;
			// se il numero di una lettera supera il numero effettivo di occorrenze (e si è certi del numero)
			if (info->characterCounter[h->index] > info->discoveredOccurrences[h->index] && info->isDefinitive[h->index])
			{
				// rendo non valido il nodo
				h->valid = 0;
			}
			else
			{
				// conto le lettere con abbastanza occorrenze
				if (info->characterCounter[h->index] == info->discoveredOccurrences[h->index])
				{
					info->lettersWithEnoughOccurrences++;
				}
				// quando raggiungo la fine della parola
				if (h->child == NULL)
				{
					if (info->lettersWithEnoughOccurrences == info->numberOfDiscoveredOccurrences)
					{
						// conto la filtrata
						info->filteredCounter++;
					}
					else
					{
						// rendo non valido il twig dalla radice morta in su e decremento i contatori
						h->valid = 0;
					}
				}
				// child
				filterOccurrencesIntwig(info, h->child);
				;
				// se il contatore della lettera, che mi garantiva che ci fossero abbastanza
				// occorrenze della lettera stessa, va sotto la soglia, non ci sono più abbastanza occorrenze
				if (info->characterCounter[h->index] == info->discoveredOccurrences[h->index])
				{
					info->lettersWithEnoughOccurrences--;
				}
			}
			// al ritorno decremento la lettera
			info->characterCounter[h->index]--;
		}
		// sibiling
		if (h->sibiling)
			filterOccurrencesIntwig(info, h->sibiling);
	}*/
}
//////////////
//	UTILS	//
//////////////
// - 0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _ a b c d e f g h i j k l m n o p q r s t u v w x y z
// 0 1 2 3 4 5 6 7 8 9 101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263
int characterToIndex(char c)
{
	// 11-36
	if (c <= 'Z' && c >= 'A')
	{
		return c - 'A' + 11;
	}
	// 38-64
	else if (c >= 'a' && c <= 'z')
	{
		return c - 'a' + 38;
	}
	// 1-10
	else if (c <= '9' && c >= '0')
	{
		return c - '0' + 1;
	}
	// 0
	else if (c == '-')
	{
		return 0;
	}
	// 37
	return 37;
}
char indexToCharacter(int n)
{
	// 11-36
	if (n <= 36 && n >= 11)
	{
		return n + 'A' - 11;
	}
	// 38-64
	else if (n >= 38 && n <= 63)
	{
		return n + 'a' - 38;
	}
	// 1-10
	else if (n <= 10 && n >= 1)
	{
		return n + '0' - 1;
	}
	// 0
	else if (n == 0)
	{
		return '-';
	}
	// 37
	return '_';
}

//////////////
//	FIND	//
//////////////
// ritorno uno se trovo la parola, zero se non la trovo
_Bool findWord(info_t *info, void *head)
{
	short int i, bodyLength, foundIndex;
	void *cursor;

	foundIndex = 0;
	cursor = head;
	bodyLength = info->length - 1;
	// scorro la parola inserita finchè trovo i figli
	for (i = 0; i < bodyLength && foundIndex != -1; i++)
	{
		// trovo il figlio e ritorno l'indice, ritorno -1 se non lo trovo
		foundIndex = findChild(cursor, info->word[i]);
		// sposto il cursore nel figlio trovato
		// il figlio ha per indirizzo l'indirizzo dell'array di figli sommato all' indice trovato
		cursor = ((twig_t *)(((twig_t *)cursor)->children) + foundIndex);
	}
	if (foundIndex != -1)
	{
		// trovo il figlio e ritorno l'indice, ritorno -1 se non lo trovo
		foundIndex = findLeaf(cursor, info->word[i]);
	}

	if (foundIndex != -1)
		return 1;
	return 0;
}
// trova e ritorna l'indirizzo del figlio se c'è, altrimenti ritorna null
int findChild(void *h, int c)
{
	short int i;
	// scorro fino alla fine o finchè non raggiungo un figlio maggiore
	for (i = 0;
		 (i <= ((twig_t *)h)->maxChildIndex) &&
		 (((twig_t *)((twig_t *)h)->children)[i]).node.index < c;
		 i++)
		;
	// se l'ho trovato
	if (i <= ((twig_t *)h)->maxChildIndex &&
		(((twig_t *)((twig_t *)h)->children)[i]).node.index == c)
	{
		return i;
	}
	return -1;
}
int findLeaf(void *h, int c)
{
	short int i;
	// scorro fino alla fine o finchè non raggiungo un figlio maggiore
	for (i = 0;
		 (i <= ((twig_t *)h)->maxChildIndex) &&
		 (((node_t *)((twig_t *)h)->children)[i]).index < c;
		 i++)
		;
	// se l'ho trovato
	if (i <= ((twig_t *)h)->maxChildIndex &&
		(((node_t *)((twig_t *)h)->children)[i]).index == c)
	{
		return i;
	}
	return -1;
}

//////////////
//	INPUT	//
//////////////
void addWords(info_t *info, void *head)
{
	short unsigned int i, j;
	char character;
	// prendi primo carattere
	do
	{
		character = getc(stdin);
	} while (character == EOS || character == EOL || character == EOW);

	// mentre non ricevo nuovi comandi
	for (i = 0; character != '+'; i++)
	{
		// acquisisco la parola al contrario
		for (j = 0; j < info->length; j++)
		{
			info->word[j] = characterToIndex(character);
			character = getc(stdin);
		}
		// la aggiungo al dizionario
		addWord(info, head);
		// prendi primo carattere della prossima riga
		character = getc(stdin);
	}
}

///////////////
//	STORAGE  //
///////////////
void *addFirstNode(void *h)
{
	// adding node
	h = (twig_t *)malloc(sizeof(twig_t));
	if (h)
	{
		// indice inutile
		((node_t *)h)->index = 0;
		// killerId inutile
		((node_t *)h)->killerId = 0;
		// profondità utile per i calcoli
		((twig_t *)h)->depth = -1;
		// ricordarsi che non è 0 ma null
		((twig_t *)h)->maxChildIndex = 0;
		((twig_t *)h)->children = NULL;
	}
	else
	{
		printf("memory error");
	}
	return h;
}
void addWord(info_t *info, void *head)
{
	short unsigned int i, bodyLength, foundIndex;
	void *cursor;

	foundIndex = 0;
	cursor = head;
	bodyLength = info->length - 1;
	// scorro la parola inserita
	for (i = 0; i < bodyLength; i++)
	{
		// trovo il figlio o lo aggiungo se non c'è, devo pure aggiornare l'head nel caso cambi
		foundIndex = findOrAddNode(cursor, info->word[i]);
		// sposto il cursore nel figlio (che sia stato appena aggiunto o meno è indifferente)
		// il figlio ha per indirizzo l'indirizzo dell'array di figli sommato all' indice trovato
		cursor = ((twig_t *)(((twig_t *)cursor)->children) + foundIndex);
	}
	// trovo il figlio o lo aggiungo se non c'è, ma non occorre ritornarne l'indirizzo stavolta, devo pure aggiornare l'head nel caso cambi
	findOrAddLeaf(cursor, info->word[i]);
}
// unica funzione che aggiunge il figlio se non c'è oppure lo trova
// ritorna l'indice del figlio in ambo i casi
int findOrAddNode(void *h, int c)
{
	int i, j;
	i = 0;
	// se ha figli lo cerco
	if (((twig_t *)h)->children != NULL)
		// scorro fino alla fine o finchè non raggiungo un figlio maggiore
		while ((i <= ((twig_t *)h)->maxChildIndex) &&
			   ((twig_t *)((twig_t *)h)->children)[i].node.index < c)
		{
			i++;
		}
	// se non l'ho trovato
	if (
		// se non aveva figli
		((twig_t *)h)->children == NULL ||
		// se ho raggiunto la lunghezza
		i > ((twig_t *)h)->maxChildIndex ||
		// se non mi sono fermato sul carattere
		(((twig_t *)((twig_t *)h)->children)[i]).node.index != c)
	{
		// se è il primo figlio che aggiungo
		if (((twig_t *)h)->children == NULL)
			// setto a zero il max child index
			((twig_t *)h)->maxChildIndex = 0;
		else
			// aumento il max child index
			((twig_t *)h)->maxChildIndex++;
		// faccio spazio
		((twig_t *)h)->children = realloc(((twig_t *)h)->children, sizeof(twig_t) * (((twig_t *)h)->maxChildIndex + 1));
		// shifto a destra gli elementi partendo dalla posizione i
		for (j = ((twig_t *)h)->maxChildIndex; j > i; j--)
		{
			((twig_t *)((twig_t *)h)->children)[j].node.index = ((twig_t *)((twig_t *)h)->children)[j - 1].node.index;
			((twig_t *)((twig_t *)h)->children)[j].node.killerId = ((twig_t *)((twig_t *)h)->children)[j - 1].node.killerId;
			((twig_t *)((twig_t *)h)->children)[j].depth = ((twig_t *)((twig_t *)h)->children)[j - 1].depth;
			((twig_t *)((twig_t *)h)->children)[j].maxChildIndex = ((twig_t *)((twig_t *)h)->children)[j - 1].maxChildIndex;
			((twig_t *)((twig_t *)h)->children)[j].children = ((twig_t *)((twig_t *)h)->children)[j - 1].children;
		}
		// memmove(((twig_t *)((twig_t *)h)->children) + i + 1, ((twig_t *)((twig_t *)h)->children) + i, sizeof(twig_t) * (((twig_t *)h)->maxChildIndex - i));
		//  assegno i valori al figlio appena aggiunto
		((twig_t *)((twig_t *)h)->children)[i].node.index = c;
		((twig_t *)((twig_t *)h)->children)[i].node.killerId = 0;
		((twig_t *)((twig_t *)h)->children)[i].depth = ((twig_t *)h)->depth + 1;
		((twig_t *)((twig_t *)h)->children)[i].maxChildIndex = 0;
		((twig_t *)((twig_t *)h)->children)[i].children = NULL;
	}
	return i;
}
// funzione che aggiunge la foglia se non c'è
void findOrAddLeaf(void *h, int c)
{
	int i, j;
	i = 0;
	// se ha figli lo cerco
	if (((twig_t *)h)->children != NULL)
		// scorro fino alla fine o finchè non raggiungo un figlio maggiore
		while ((i <= ((twig_t *)h)->maxChildIndex) &&
			   ((((node_t *)((twig_t *)h)->children)[i]).index < c))
		{
			i++;
		}
	// se non l'ho trovato
	if (
		// se non aveva figli
		((twig_t *)h)->children == NULL ||
		// se ho raggiunto la lunghezza
		i > ((twig_t *)h)->maxChildIndex ||
		// se non mi sono fermato sul carattere
		(((node_t *)((twig_t *)h)->children)[i]).index != c)
	{
		// se è il primo figlio che aggiungo
		if (((twig_t *)h)->children == NULL)
			// setto a zero il max child index
			((twig_t *)h)->maxChildIndex = 0;
		else
			// aumento il max child index
			((twig_t *)h)->maxChildIndex++;
		// faccio spazio
		((twig_t *)h)->children = realloc(((twig_t *)h)->children, sizeof(node_t) * (((twig_t *)h)->maxChildIndex + 1));
		// shifto a destra gli elementi partendo dalla posizione i
		for (j = ((twig_t *)h)->maxChildIndex; j > i; j--)
		{
			((node_t *)((twig_t *)h)->children)[j].index = ((node_t *)((twig_t *)h)->children)[j - 1].index;
			((node_t *)((twig_t *)h)->children)[j].killerId = ((node_t *)((twig_t *)h)->children)[j - 1].killerId;
		}
		// memmove(((node_t *)((twig_t *)h)->children) + i + 1, ((node_t *)((twig_t *)h)->children) + i, sizeof(node_t) * (((twig_t *)h)->maxChildIndex - i));
		//  assegno i valori al figlio appena aggiunto
		((node_t *)((twig_t *)h)->children)[i].index = c;
		((node_t *)((twig_t *)h)->children)[i].killerId = 0;
	}
}

//////////////
//	FILTER  //
//////////////
void filterDictionary(info_t *info, void *h)
{
	int i;
	// inizializzo il contatore di caratteri a zero
	for (i = 0; i < ALPH_LEN; i++)
	{
		info->characterCounter[i] = 0;
	}
	// inizializzo il numero di lettere che hanno abbastanza occorrenze
	info->lettersWithEnoughOccurrences = 0;
	// inizializzo il contatore delle filtrate a zero
	info->filteredCounter = 0;
	// se i figli sono rami
	if ((((twig_t *)h)->depth + 1) < info->maxDepth)
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				filterNode(info, ((twig_t *)((twig_t *)h)->children) + i);
			}
		}
	}
	// se i figli sono foglie
	else
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				filterLeaf(info, ((node_t *)((twig_t *)h)->children) + i);
			}
		}
	}
	// è tutto uptodate
	info->isDiscoveredOccurrencesUTD = 1;
	info->isIsPositionOfCharacterValidUTD = 1;
}
void filterNode(info_t *info, void *h)
{
	short unsigned int i;
	// conto i caratteri
	info->characterCounter[((node_t *)h)->index]++;
	// se il numero di una lettera supera il numero effettivo di occorrenze (e si è certi del numero)
	if (info->characterCounter[((node_t *)h)->index] > info->discoveredOccurrences[((node_t *)h)->index] && info->isDefinitive[((node_t *)h)->index])
	{
		// rendo non valido il nodo
		((node_t *)h)->killerId = info->matchId;
	}
	else
	{
		// se il carattere non si può trovare a quella posizione (profondità)
		if (!(info->isPositionOfCharacterValid[((twig_t *)h)->depth][((node_t *)h)->index]))
		{
			// rendo non valido il nodo
			((node_t *)h)->killerId = info->matchId;
		}
		else
		{
			// conto le lettere con abbastanza occorrenze
			if (info->characterCounter[((node_t *)h)->index] == info->discoveredOccurrences[((node_t *)h)->index])
			{
				info->lettersWithEnoughOccurrences++;
			}
			// se i figli sono rami
			if ((((twig_t *)h)->depth + 1) < info->maxDepth)
			{
				for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
				{
					if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
					{
						filterNode(info, ((twig_t *)((twig_t *)h)->children) + i);
					}
				}
			}
			// se i figli sono foglie
			else
			{
				for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
				{
					if (((node_t *)(((node_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
					{
						filterLeaf(info, ((node_t *)((twig_t *)h)->children) + i);
					}
				}
			}
			// se il contatore della lettera, che mi garantiva che ci fossero abbastanza
			// occorrenze della lettera stessa, va sotto la soglia, non ci sono più abbastanza occorrenze
			if (info->characterCounter[((node_t *)h)->index] == info->discoveredOccurrences[((node_t *)h)->index])
			{
				info->lettersWithEnoughOccurrences--;
			}
		}
	}
	// al ritorno decremento la lettera
	info->characterCounter[((node_t *)h)->index]--;
}
void filterLeaf(info_t *info, void *h)
{
	// conto i caratteri
	info->characterCounter[((node_t *)h)->index]++;
	// se il numero di una lettera supera il numero effettivo di occorrenze (e si è certi del numero)
	if (info->characterCounter[((node_t *)h)->index] > info->discoveredOccurrences[((node_t *)h)->index] && info->isDefinitive[((node_t *)h)->index])
	{
		// rendo non valido il nodo
		((node_t *)h)->killerId = info->matchId;
	}
	else
	{
		// se il carattere non si può trovare a quella posizione (profondità)
		if (!(info->isPositionOfCharacterValid[info->maxDepth][((node_t *)h)->index]))
		{
			// rendo non valido il nodo
			((node_t *)h)->killerId = info->matchId;
		}
		else
		{
			// conto le lettere con abbastanza occorrenze
			if (info->characterCounter[((node_t *)h)->index] == info->discoveredOccurrences[((node_t *)h)->index])
			{
				info->lettersWithEnoughOccurrences++;
			}
			// quando raggiungo la fine della parola
			if (info->lettersWithEnoughOccurrences == info->numberOfDiscoveredOccurrences)
			{
				// conto la filtrata
				info->filteredCounter++;
			}
			else
			{
				// rendo non valido il twig dalla radice morta in su e decremento i contatori
				((node_t *)h)->killerId = info->matchId;
			}
			// se il contatore della lettera, che mi garantiva che ci fossero abbastanza
			// occorrenze della lettera stessa, va sotto la soglia, non ci sono più abbastanza occorrenze
			if (info->characterCounter[((node_t *)h)->index] == info->discoveredOccurrences[((node_t *)h)->index])
			{
				info->lettersWithEnoughOccurrences--;
			}
		}
	}
	// al ritorno decremento la lettera
	info->characterCounter[((node_t *)h)->index]--;
}

//////////////
//	OUTPUT	//
//////////////
void printDictionary(info_t *info, void *h)
{
	short unsigned int i;

	info->word[info->length - 1] = EOS;

	// se i figli sono rami
	if ((((twig_t *)h)->depth + 1) < info->maxDepth)
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				printNode(info, ((twig_t *)((twig_t *)h)->children) + i);
			}
		}
	}
	// se i figli sono foglie
	else
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				printLeaf(info, ((node_t *)((twig_t *)h)->children) + i);
			}
		}
	}
}
void printNode(info_t *info, void *h)
{
	short unsigned int i;

	// posiziona il carattere nella parola contenitore
	info->result[((twig_t *)h)->depth] = indexToCharacter(((node_t *)h)->index);
	// stampa le foglie
	// se i figli sono rami
	if ((((twig_t *)h)->depth + 1) < info->maxDepth)
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((twig_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				printNode(info, ((twig_t *)((twig_t *)h)->children) + i);
			}
		}
	}
	// se i figli sono foglie
	else
	{
		for (i = 0; i <= ((twig_t *)h)->maxChildIndex; i++)
		{
			if (((node_t *)(((node_t *)((twig_t *)h)->children) + i))->killerId != info->matchId)
			{
				printLeaf(info, ((node_t *)((twig_t *)h)->children) + i);
			}
		}
	}
}
void printLeaf(info_t *info, void *h)
{
	// posiziona il carattere nella parola contenitore
	info->result[info->maxDepth] = indexToCharacter(((node_t *)h)->index);
	// stampa la parola quando raggiungi la foglia
	printf("%.*s\n", info->length, info->result);
}
void compareWords(info_t *info, _Bool *isFree)
{
	short unsigned int i, j, found, definitive;

	for (i = 0; i < info->length; i++)
	{
		if (info->word[i] == info->solution[i])
		{
			// giusta al posto giusto
			info->result[i] = '+';
			// segno che è già stata associata
			isFree[i] = 0;
			// metto a zero tutti i caratteri (nessuno è valido tranne quello giusto al posto giusto)
			for (j = 0; j < ALPH_LEN; j++)
			{
				// se non l'avevo precedentemente messo a zero lo azzero, ovviamente ciò non vale per quello giusto al posto giusto
				if (info->isPositionOfCharacterValid[i][j] == 1 && j != info->word[i])
				{
					// azzero la posizione (non è valida)
					info->isPositionOfCharacterValid[i][j] = 0;
					// segno che sono cambiate le posizioni
					info->isIsPositionOfCharacterValidUTD = 0;
				}
			}
			// conto la lettera
			info->characterCounter[info->word[i]]++;
		}
		else
		{
			// per ora non è stata trovata
			info->result[i] = '/';
			// non è associata
			isFree[i] = 1;
			// i caratteri che non sono risultati giusti al posto giusto
			// controllo il valore (vedo se sono già rimossi)
			if (info->isPositionOfCharacterValid[i][info->word[i]] == 1)
			{
				// se non lo sono li rimuovo
				info->isPositionOfCharacterValid[i][info->word[i]] = 0;
				// segno che sono cambiate le posizioni
				info->isIsPositionOfCharacterValidUTD = 0;
			}
		}
	}
	for (i = 0; i < info->length; i++)
	{
		definitive = 0;
		found = 0;
		for (j = 0; j < info->length && !found; j++)
		{
			// controllo se esiste una uguale non associata e non fissata (ancora '/' e non '+')
			if (info->word[i] == info->solution[j] && info->result[i] != '+')
			{
				// se trovo una non associata
				if (isFree[j])
				{
					// trovata, devo passare alla prossima lettera
					// anche per non contare le altre
					found = 1;
					// giusta al posto sbagliato
					info->result[i] = '|';
					// conto le lettere
					info->characterCounter[info->word[i]]++;
					// segno che è già stata associata
					isFree[j] = 0;
					// se prima pensavo fosse definitiva ho scoperto che non lo è
					definitive = 0;
				}
				// potrei trovare una da associare dopo, ma se non la trovassi avrei le occorrenze definitive
				else
				{
					definitive = 1;
				}
			}
		}
		if (definitive)
		{
			// se nin è già definitiva
			if (info->isDefinitive[info->word[i]] == 0)
			{
				// metto il numero corretto di occorrenze
				info->discoveredOccurrences[info->word[i]] = info->trueOccurrences[info->word[i]];
				// segno che è definitivo
				info->isDefinitive[info->word[i]] = 1;
				// segno che sono variati i definitivi
				// ma se sono variati i definitivi sono variate le occorrenze, non ha senso averli separati
				info->isDiscoveredOccurrencesUTD = 0;
			}
		}
		// controllo se viene trovata
		found = 0;
		for (j = 0; j < info->length && !found; j++)
		{
			// controllo se viene trovata
			if (info->word[i] == info->solution[j])
			{
				// segno che è stata trovata e posso uscire dal ciclo
				found = 1;
			}
		}
		// se questo è vero, è proprio sbagliata!!, la rimuovo in toto (da ogni posizione)
		if (!found)
		{
			for (j = 0; j < info->length; j++)
			{
				// controllo se la posizione è ancora valida
				if (info->isPositionOfCharacterValid[j][info->word[i]] == 1)
				{
					// se lo è la rendo non valida
					info->isPositionOfCharacterValid[j][info->word[i]] = 0;
					// segno che sono variate le posizioni
					info->isIsPositionOfCharacterValidUTD = 0;
				}
			}
		}
	}
	info->numberOfDiscoveredOccurrences = 0;
	// aggiorno le occorrenze che non sono già definitive
	for (int i = 0; i < ALPH_LEN; i++)
	{
		// se non ho già stabilito il numero definitivo di occorrenze per questa lettera
		if (!(info->isDefinitive[i]))
		{
			// non c'è pericolo di contare più occorrenze di quelle massime
			// ogni volta controllo se è stata associata la lettera
			if (info->characterCounter[i] > info->discoveredOccurrences[i])
			{
				// aggiorno il minimo di occorrenze
				info->discoveredOccurrences[i] = info->characterCounter[i];
				// segno che sono variate le occorrenze
				info->isDiscoveredOccurrencesUTD = 0;
			}
		}
		// aggiorno il numero di occorrenze scoperte
		if (info->discoveredOccurrences[i] > 0)
		{
			info->numberOfDiscoveredOccurrences++;
		}
	}
	// stampa risultato del confronto
	printf("%.*s\n", info->length, info->result);

	// mostro caratteri che posso trovare alle posizioni
	/*
		for (i = 0; i < info->length; i++)
		{
			for (j = 0; j < ALPH_LEN; j++)
				printf("-%d-", info->isPositionOfCharacterValid[i][j]);
			printf("\n");
		}
		printf("\n");
		printf("\ncounter\n");
		for (i = 0; i < ALPH_LEN; i++)
		{
			printf("%d", info->characterCounter[i]);
		}
		printf("\ndiscovered\n");
		for (i = 0; i < ALPH_LEN; i++)
		{
			printf("%d", info->discoveredOccurrences[i]);
		}
		printf("\ndefinitive\n");
		for (i = 0; i < ALPH_LEN; i++)
		{
			printf("%d", info->isDefinitive[i]);
		}
		printf("\ntrue\n");
		for (i = 0; i < ALPH_LEN; i++)
		{
			printf("%d", info->trueOccurrences[i]);
		}
		printf("\n");
		printf("\number of discovered\n");
		printf("%d", info->numberOfDiscoveredOccurrences);
		printf("\n");
	*/
	return;
}

//////////////
//	MAIN	//
//////////////
void startMatch(info_t *info, void *head, _Bool *isFree)
{
	short unsigned int i, j, attempts;
	char character;
	char command[MAX_COMMAND_LENGTH];

	info->matchId++;
	//  prendo la soluzione
	for (i = 0; i < info->length; i++)
		info->solution[i] = characterToIndex(getc(stdin));
	// prendo il null
	character = getc(stdin);
	// scan per sapere quanti tentativi ho a disposizione in questa partita
	if (scanf("%hd", &attempts))
		;
	//  prendi primo carattere
	do
	{
		character = getc(stdin);
	} while (character == EOS || character == EOL || character == EOW);

	// inizializzo i caratteri che posso trovare alle posizioni
	for (i = 0; i < info->length; i++)
		for (j = 0; j < ALPH_LEN; j++)
			info->isPositionOfCharacterValid[i][j] = 1;
	// inizializzo il numero di caratteri trovati
	for (i = 0; i < ALPH_LEN; i++)
		info->discoveredOccurrences[i] = 0;
	// inizializzo il numero di caratteri della soluzione
	for (i = 0; i < ALPH_LEN; i++)
		info->trueOccurrences[i] = 0;
	// conto i caratteri della soluzione
	for (i = 0; i < info->length; i++)
		info->trueOccurrences[info->solution[i]]++;
	// inizializzo i definitivi
	for (i = 0; i < ALPH_LEN; i++)
		info->isDefinitive[i] = 0;
	// inizializzo le occorrenze scoperte
	info->numberOfDiscoveredOccurrences = 0;
	info->isDiscoveredOccurrencesUTD = 0;
	info->isIsPositionOfCharacterValidUTD = 0;
	// le parole provate sono minori del numero di tentativi
	i = 0;
	while (i < attempts)
	{
		// controllo se ho ricevuto comandi durante una partita
		if (character == '+')
		{
			if (fgets(command, MAX_COMMAND_LENGTH, stdin))
				;
			// comando per stampare filtrate durante una partita
			if (strcmp(command, "stampa_filtrate\n") == 0)
			{
				// printf("\n---printing_dictionary---\n");
				printDictionary(info, head);
			}
			// comando per aggiungere parole al dizionario durante una partita
			else if (strcmp(command, "inserisci_inizio\n") == 0)
			{
				addWords(info, head);
				// devo "buttare" la parte finale di comando ("inserisci fine")
				if (fgets(command, MAX_COMMAND_LENGTH, stdin))
					;
				filterDictionary(info, head);
			}
		}
		else
		{
			// acquisisco la parola
			for (j = 0; j < info->length; j++)
			{
				info->word[j] = characterToIndex(character);
				character = getc(stdin);
			}
			// controllo se la parola è presente nel dizionario
			if (findWord(info, head))
			{
				// incremento il contatore di tentativi (valgono solo i tentativi validi)
				i++;
				// vedo se la parola per caso è giusta
				for (j = 0; info->word[j] == info->solution[j]; j++)
					if (j == (info->length - 1))
					{
						puts("ok");
						return;
					}
				// inizializzo il contatore di caratteri a zero
				for (j = 0; j < ALPH_LEN; j++)
				{
					info->characterCounter[j] = 0;
				}
				// inizializzo i free
				for (j = 0; j < info->length; j++)
					isFree[j] = 0;
				// paragono la parola alla soluzione, stampo il risultato del confronto e aggiorno info
				compareWords(info, isFree);

				// se nessuno dei due è UTD
				if ((info->isDiscoveredOccurrencesUTD && info->isIsPositionOfCharacterValidUTD))
				{
				}
				else
				{
					filterDictionary(info, head);
				}
				/* se le occorrenze non sono UTD
				else if (!(info->isDiscoveredOccurrencesUTD))
				{
					filterOccurrences(info, head);
				}
				else if (!(info->isIsPositionOfCharacterValidUTD))
				{
					filterPositions(info, head);
				}*/
				// stampa numero di parole rimaste
				printf("%d\n", info->filteredCounter);
			}
			else
			{
				puts("not_exists");
			}
		}
		// se ho tentativi rimanenti
		if (i < attempts)
			// prendi primo carattere della prossima riga
			character = getc(stdin);
	}
	if (i >= attempts)
	{
		// printf("ultimo car: %c\n", character);
		puts("ko");
	}

	return;
}
int main(int argc, char *argv[])
{
	short unsigned int wordLength;
	info_t infoVar;
	twig_t *dictionary = NULL;
	char character;
	_Bool *isFree;
	char command[MAX_COMMAND_LENGTH + 1];
	/*
		printf("%ld\n", sizeof(info_t));
		printf("%ld\n", sizeof(twig_t));
		printf("%ld\n", sizeof(branched_twig_t));
		printf("%ld\n", sizeof(leaf_t));
	*/
	// scan per sapere quanto è lunga la parola
	if (scanf("%hd", &wordLength))
		;

	infoVar.length = wordLength;
	infoVar.maxDepth = wordLength - 1;
	infoVar.matchId = 0;
	// creo dizionario vuoto
	dictionary = addFirstNode(dictionary);

	// creo vettore di vettori statici
	infoVar.isPositionOfCharacterValid = malloc(sizeof(_Bool) * wordLength * ALPH_LEN);
	// creating array where to put infoVar.word
	infoVar.word = (int *)malloc(7 * wordLength);
	// creo vettore dove mettere la soluione
	infoVar.solution = (int *)malloc(7 * wordLength);
	// creo vettore dove mettere il risultato del confronto
	infoVar.result = (char *)malloc(sizeof(char) * wordLength);
	// creating array where to put check for pairing
	isFree = (_Bool *)malloc(sizeof(_Bool) * wordLength);

	if (infoVar.word && infoVar.solution && infoVar.result && isFree && infoVar.isPositionOfCharacterValid)
	{
		// aggiungo per la prima volta parole al dizionario
		addWords(&infoVar, dictionary);

		// sicuramente ho terminato con un comando
		// il carattere '+' con cui inizia il comando nuova_partita è stato mangiato da addWords che lo ha "scambiato" per il '+' di inserisci_fine
		character = '+';
		// se non sono in una partita e non sto aggiungendo parole, posso solo ricevere comandi, ma controlliamo ugualmente per sicurezza
		while (character == '+')
		{
			if (fgets(command, MAX_COMMAND_LENGTH, stdin))
				;
			// printf("\ncomando: %s", command);
			//  comando per iniziare una partita
			if (strcmp(command, "nuova_partita\n") == 0)
			{
				startMatch(&infoVar, dictionary, isFree);
			}
			// comando per aggiungere parole al dizionario tra una partita e l'altra
			else if (strcmp(command, "inserisci_inizio\n") == 0)
			{
				addWords(&infoVar, dictionary);
				// devo "buttare" la parte finale di comando ("inserisci fine")
				if (fgets(command, MAX_COMMAND_LENGTH, stdin))
					;
			}
			// prendi primo carattere (tendenzialmente sarà un '+'), se non lo è termino l'esecuzione (sarà un \n)
			character = getc(stdin);
		}
	}
	else
	{
		printf("memory error");
	}

	return 0;
}

// finirò questo con le classi implementate in c

// da provare una soluzione senza liste di figli ma come un array di puntatori del quale faccio realloc ogni volta.
// consumerà un sacco in immissione, ma dopo sarà veloce