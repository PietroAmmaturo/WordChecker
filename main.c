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

// tipi
//  struttura dati contenente le informazioni relative ai vari inserimanti
typedef struct info_s
{
	// 0 contenente il numero minimo di occorrenze nella parola (sia incognite che fissate)
	// pari a -1 se si è scoperto che la lettera non è presente..
	//'A' è in posizione 0, 'B' in posizione 1, eccetera
	int discoveredOccurrences[ALPH_LEN];

	// occorrenze delle lettere nella parola
	int trueOccurrences[ALPH_LEN];

	// indica per ogni lettera se il numero di occorrenze è il numero definitivo.
	// Il numero di occorrenze è definitivo se ho testato che una parola ha una lettera "sbagliata"
	// dopo aver scoperto che la stessa lettera era "giusta al posto sbagliato" o "giusta al posto giusto" in una posizione precedente
	// o se il contatore di una lettera è maggiore del numero di lettere effettive
	_Bool isDefinitive[ALPH_LEN];

	//(contenente la lista di possibili caratteri che si possono trovare in quella posizione, organizzato come un array di bool.
	// NB: se un carattere è fissato in una posizione (giusto al posto giusto), sarà presente solo lui,
	// se invece un carattere in quella posizione è risultato "giusto al posto sbagliato", sicuramente non sarà presente in quella posizione)
	_Bool (*isPositionOfCharacterValid)[ALPH_LEN];

	// indica se c'è bisogno di aggiornare il valido dei nodi
	_Bool toUpdate;

} info_t;

// come strutturo un nodo dell'albero rappresentante il dizionario
typedef struct tree_s
{
	struct tree_s *parent;
	// profondità a cui si trova un nodo
	int depth;
	// carattere contenuto nel nodo
	char character;
	// figli del nodo (una lista dinamica)
	struct list_s *child;
} tree_t;

// come strutturo un nodo della lista rappresentante i figli di ogni nodo
typedef struct list_s
{
	// puntatore al nodo
	tree_t *node;
	// puntatore al prossimo fratello
	struct list_s *nextSibiling;
} list_t;

// dichiarazioni

int characterToIndex(char c);

list_t *findChild(list_t *h, char c, tree_t *p);

_Bool findWord(tree_t *head, char *word, int length);

tree_t *addNode(tree_t *h, char c, int d, tree_t *p);

list_t *addChildBetween(list_t *last, list_t *next, char c, int d, tree_t *p);

list_t *findOrAddChild(list_t *h, char c, int d, tree_t *p, list_t **found);

tree_t *addWord(tree_t *head, char *word, int length);

tree_t *addWords(tree_t *head, char *word, int length);

void printDictionary(tree_t *h, char *word, int length);

void updateOccurrences(info_t *info, int *counters);

void removeCharacterAtPosition(info_t *info, char c, int position);

void removeAllCharactersAtPositionExcept(info_t *info, char c, int position);

void startMatch(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length);

void compareWords(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length);

//////////////
//	utils	//
//////////////
int characterToIndex(char c)
{
	// 10-36
	if (c <= 'Z' && c >= 'A')
	{
		return c - 'A' + 10;
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
	else if (c == '_')
	{
		return 37;
	}

	printf("error");
	return -1;
}
_Bool findWord(tree_t *head, char *word, int length)
{
	int i;
	tree_t *treeCursor;
	list_t *rightChild;

	printf("\nfinding: \n");
	for (i = 0; i < length; i++)
		printf("%c", word[i]);

	treeCursor = head;

	// scorro la parola inserita
	for (i = 0; i < length; i++)
	{
		// trovo il figlio, devo pure aggiornare l'head nel caso cambi
		rightChild = findChild(treeCursor->child, word[i], treeCursor);
		// se lo trovo proseguo la scansione
		if (rightChild)
			// sposto il cursore nel figlio trovato
			treeCursor = rightChild->node;
		else
			// se non lo trovo termino ritornando 0
			return 0;
	}
	return 1;
}
//////////////
//	info	//
//////////////
void updateOccurrences(info_t *info, int *counters)
{
	printf("\n----updating occurrences----\n");

	for (int i = 0; i < ALPH_LEN; i++)
	{
		// se non ho già stabilito il numero definitivo di occorrenze per questa lettera
		if (!(info->isDefinitive[i]))
		{
			// se ho contato più occorrenze di quelle massime, mi sarò di sicuro accorto del numero esatto di occorrenze
			if (counters[i] > info->trueOccurrences[i])
			{
				info->discoveredOccurrences[i] = info->trueOccurrences[i];
				info->isDefinitive[i] = 1;
			}
			// se ho contato più occorrenze del precedente valore minimo, allora ho trovato un valore minimo di occorrenze più alto (maggior precisione)
			else if (counters[i] > info->discoveredOccurrences[i])
			{
				info->discoveredOccurrences[i] = counters[i];
			}
		}
		printf("-%d-", info->discoveredOccurrences[i]);
	}
	printf("\n");
}
void removeCharacterAtPosition(info_t *info, char c, int position)
{
	printf("\n removing: %c at position %d", c, position);
	info->isPositionOfCharacterValid[position][characterToIndex(c)] = 0;
}
void removeAllCharactersAtPositionExcept(info_t *info, char c, int position)
{
	printf("\n removing all except: %c at position %d", c, position);
}

//////////////
//	alberi	//
//////////////
tree_t *addNode(tree_t *h, char c, int d, tree_t *p)
{
	// adding node
	h = (tree_t *)malloc(sizeof(tree_t));
	if (h)
	{
		h->parent = p;
		h->character = c;
		h->depth = d;
		h->child = NULL;
	}
	else
	{
		printf("memory error");
	}
	return h;
}
tree_t *addWord(tree_t *head, char *word, int length)
{
	int i;
	tree_t *treeCursor;
	list_t *rightChild;

	treeCursor = head;

	// scorro la parola inserita
	for (i = 0; i < length; i++)
	{
		// trovo il figlio o lo aggiungo se non c'è, devo pure aggiornare l'head nel caso cambi
		treeCursor->child = findOrAddChild(treeCursor->child, word[i], i, treeCursor, &rightChild);
		// sposto il cursore nel figlio (che sia stato appena aggiunto o meno è indifferente)
		treeCursor = rightChild->node;
	}
	return head;
}
tree_t *addWords(tree_t *head, char *word, int length)
{
	printf("\n----inizio inserimento----\n");
	int i, j;
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
		for (j = 0; j < length; j++)
		{
			word[j] = character;
			character = getc(stdin);
		}
		// la aggiungo al dizionario
		addWord(head, word, length);
		// prendi primo carattere della prossima riga
		character = getc(stdin);
	}
	printf("\n----fine inserimento----\n");

	return head;
}

//////////////
//	liste	//
//////////////

// unica funzione che aggiunge il figlio se non c'è
// oppure ritorna l'indirizzo del figlio se c'è
list_t *findOrAddChild(list_t *h, char c, int d, tree_t *p, list_t **found)
{
	list_t *cursor, *previous;
	previous = NULL;
	// sposto il cursore fino a trovare il figlio che punta a un nodo il cui carattere è maggiore o uguale di quello desiderato
	for (cursor = h; cursor != NULL && cursor->node->character < c; cursor = cursor->nextSibiling)
		previous = cursor;
	// se non lo trovo lo aggiungo
	if (cursor == NULL || cursor->node->character > c)
	{
		cursor = addChildBetween(previous, cursor, c, d, p);
	}
	// se il precedente è null, ho aggiunto all'inizio della lista cambiando l'head
	if (previous == NULL)
	{
		h = cursor;
	}
	// in ogni caso ritorno cursor, se non l' ho aggiunto significa che:
	// cursor != NULL && cursor->node->character == c
	*found = cursor;
	return h;
}
// aggiunge elemento alla lista tra precedente e prossimo
list_t *addChildBetween(list_t *last, list_t *next, char c, int d, tree_t *p)
{
	list_t *newChild;

	newChild = (list_t *)malloc(sizeof(list_t));
	// adding node
	if (newChild)
	{
		newChild->nextSibiling = next;
		newChild->node = addNode(newChild->node, c, d, p);
		// se c'era effettivamente un nodo prima di newChild
		if (last)
		{
			last->nextSibiling = newChild;
		}
	}
	else
	{
		printf("memory error");
	}
	return newChild;
}
// trova e ritorna l'indirizzo del figlio se c'è, altrimenti ritorna null
list_t *findChild(list_t *h, char c, tree_t *p)
{
	list_t *cursor;
	// sposto il cursore fino a trovare il figlio che punta a un nodo il cui carattere è maggiore o uguale di quello desiderato
	for (cursor = h; cursor != NULL && cursor->node->character < c; cursor = cursor->nextSibiling)
		;
	// se non lo trovo ritorno null, non serve scorrerli tutti
	if (cursor == NULL || cursor->node->character > c)
	{
		cursor = NULL;
	}
	// in ogni caso ritorno cursor
	return cursor;
}

//////////////
//	I/O	//
//////////////
void printDictionary(tree_t *h, char *word, int length)
{
	list_t *listCursor;
	tree_t *threeCursor;

	threeCursor = h;

	// posiziona il carattere nella parola contenitore
	word[threeCursor->depth] = threeCursor->character;

	// stampa la parola quando raggiungi la fine
	if (threeCursor->child == NULL)
		printf("%.*s\n", length, word);

	// All the children
	for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
		printDictionary(listCursor->node, word, length);
}
void compareWords(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length)
{
	int i, j, found, characterCounters[ALPH_LEN];
	printf("\ncomparing:\n");
	printf("%.*s\n", length, word);
	printf("%.*s\n", length, solution);

	for (i = 0; i < ALPH_LEN; i++)
	{
		characterCounters[i] = 0;
	}
	for (i = 0; i < length; i++)
	{
		// nessuna è associata
		isFree[i] = 1;
		// nessuna è stata trovata
		result[i] = '/';
	}
	for (i = 0; i < length; i++)
	{
		if (word[i] == solution[i])
		{
			// giusta al posto giusto
			result[i] = '+';
			// segno che è già stata associata
			isFree[i] = 0;
			// metto a zero tutti i caratteri (nessuno è valido tranne quello giusto al posto giusto)
			for (j = 0; j < ALPH_LEN; j++)
			{
				info->isPositionOfCharacterValid[i][j] = 0;
			}
			// metto ad 1 quello giusto al posto giusto
			info->isPositionOfCharacterValid[i][characterToIndex(word[i])] = 1;
			// conto la lettera
			characterCounters[characterToIndex(word[i])]++;
		}
		else
		{
			// i caratteri che non sono risultati giusti al posto giusto li rimuovo
			info->isPositionOfCharacterValid[i][characterToIndex(word[i])] = 0;
		}
	}
	for (i = 0; i < length; i++)
	{
		// controllo se viene trovata
		found = 0;
		for (j = 0; j < length && !found; j++)
		{
			// controllo se esiste una non associata
			if (word[i] == solution[j] && isFree[i] && isFree[j])
			{
				// giusta al posto sbagliato
				result[i] = '|';
				// conto le lettere
				characterCounters[characterToIndex(word[i])]++;
				// segno che è già stata associata
				isFree[j] = 0;
				// segno che è stata trovata
				found = 1;
			}
		}
	}
	updateOccurrences(info, characterCounters);
	// inizializzo i caratteri che posso trovare alle posizioni
	for (i = 0; i < length; i++)
	{
		for (j = 0; j < ALPH_LEN; j++)
			printf("-%d-", info->isPositionOfCharacterValid[i][j]);
		printf("\n");
	}
	printf("\n");

	printf("%.*s\n", length, result);
	return;
}
void startMatch(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length)
{
	int i, j, attempts;
	char character;
	char command[MAX_COMMAND_LENGTH];

	printf("\n----inizio partita----\n");
	if (solution)
	{
		// prendo la soluzione
		for (i = 0; i < length; i++)
			solution[i] = getc(stdin);
		// prendo il null
		character = getc(stdin);
		// scan per sapere quanti tentativi ho a disposizione in questa partita
		scanf("%d", &attempts);
		// prendi primo carattere
		do
		{
			character = getc(stdin);
		} while (character == EOS || character == EOL || character == EOW);
		// inizializzo i caratteri che posso trovare alle posizioni
		for (i = 0; i < length; i++)
			for (j = 0; j < ALPH_LEN; j++)
				info->isPositionOfCharacterValid[i][j] = 1;
		// inizializzo il numero di caratteri trovati
		for (i = 0; i < ALPH_LEN; i++)
			info->discoveredOccurrences[i] = 0;
		// inizializzo il numero di caratteri della soluzione
		for (i = 0; i < ALPH_LEN; i++)
			info->trueOccurrences[i] = 0;
		// conto i caratteri della soluzione
		for (i = 0; i < length; i++)
			info->trueOccurrences[characterToIndex(solution[i])]++;
		// inizializzo i definitivi
		for (i = 0; i < ALPH_LEN; i++)
			info->isDefinitive[i] = 0;
		// le parole provate sono minori del numero di tentativi
		i = 0;
		while (i < attempts)
		{
			// controllo se ho ricevuto comandi durante una partita
			if (character == '+')
			{
				fgets(command, MAX_COMMAND_LENGTH, stdin);
				// comando per stampare filtrate durante una partita
				if (strcmp(command, "stampa_filtrate\n") == 0)
				{
					printf("\n---printing_dictionary---\n");
					printDictionary(head, word, length);
				}
				// comando per aggiungere parole al dizionario durante una partita
				else if (strcmp(command, "inserisci_inizio\n") == 0)
				{
					head = addWords(head, word, length);
					// devo "buttare" la parte finale di comando ("inserisci fine")
					fgets(command, MAX_COMMAND_LENGTH, stdin);
				}
			}
			else
			{
				// acquisisco la parola
				for (j = 0; j < length; j++)
				{
					word[j] = character;
					character = getc(stdin);
				}
				// controllo se la parola è presente nel dizionario
				if (findWord(head, word, length))
				{
					// incremento il contatore di tentativi (valgono solo i tentativi validi)
					i++;
					if (strncmp(word, solution, length) == 0)
					{
						printf("\nok");
						return;
					}
					// paragono la parola alla soluzione e aggiorno le info
					compareWords(info, head, word, solution, result, isFree, length);
				}
				else
				{
					printf("\nnot_exists");
				}
			}
			// se ho tentativi rimanenti
			if (i < attempts)
				// prendi primo carattere della prossima riga
				character = getc(stdin);
		}
		if (i >= attempts)
		{
			printf("\nultimo car: %c", character);
			printf("\nko");
			printf("\n----fine partita----\n");
		}
	}
	else
	{
		printf("memory error");
	}

	return;
}
int main(int argc, char *argv[])
{
	int wordLength;

	info_t infoVar;
	tree_t *dictionary = NULL;
	char character;
	char *word, *solution, *result;
	_Bool *isFree;
	char command[MAX_COMMAND_LENGTH + 1];

	// scan per sapere quanto è lunga la parola
	scanf("%d", &wordLength);

	// creo dizionario vuoto
	dictionary = addNode(dictionary, '\0', (wordLength - 1), NULL);

	// creo vettore di vettori statici
	infoVar.isPositionOfCharacterValid = malloc(sizeof(_Bool) * wordLength * ALPH_LEN);
	// creating array where to put word
	word = (char *)malloc(sizeof(char) * wordLength);
	// creo vettore dove mettere la soluione
	solution = (char *)malloc(sizeof(char) * wordLength);
	// creo vettore dove mettere il risultato del confronto
	result = (char *)malloc(sizeof(char) * wordLength);
	// creating array where to put check for pairing
	isFree = (_Bool *)malloc(sizeof(_Bool) * wordLength);

	if (word && solution && result && isFree && infoVar.isPositionOfCharacterValid)
	{
		// aggiungo per la prima volta parole al dizionario
		dictionary = addWords(dictionary, word, wordLength);

		// sicuramente ho terminato con un comando di inizio partita, non avrebbe senso stampare filtrate o dire di voler aggiungere parole al dizionario
		// inoltre il carattere '+' con cui inizia il comando nuova_partita è stato mangiato da addWords che lo ha "scambiato" per il '+' di inserisci_fine
		fgets(command, MAX_COMMAND_LENGTH, stdin);
		// comando per iniziare una partita
		if (strcmp(command, "nuova_partita\n") == 0)
		{
			startMatch(&infoVar, dictionary, word, solution, result, isFree, wordLength);
		}
		// prendi primo carattere (tendenzialmente sarà un '+')
		do
		{
			character = getc(stdin);
		} while (character == EOS || character == EOL || character == EOW);
		printf("\ncarattere: %c", character);

		// se non sono in una partita e non sto aggiungendo parole, posso solo ricevere comandi, ma controlliamo ugualmente per sicurezza
		while (character == '+')
		{
			fgets(command, MAX_COMMAND_LENGTH, stdin);
			printf("\ncomando: %s", command);
			// comando per iniziare una partita
			if (strcmp(command, "nuova_partita\n") == 0)
			{
				startMatch(&infoVar, dictionary, word, solution, result, isFree, wordLength);
			}
			// comando per aggiungere parole al dizionario tra una partita e l'altra
			else if (strcmp(command, "inserisci_inizio\n") == 0)
			{
				dictionary = addWords(dictionary, word, wordLength);
				// devo "buttare" la parte finale di comando ("inserisci fine")
				fgets(command, MAX_COMMAND_LENGTH, stdin);
			}
			// prendi primo carattere (tendenzialmente sarà un '+')
			character = getc(stdin);
		}
	}
	else
	{
		printf("memory error");
	}

	return 0;
}

// si può fare probabilemnte a meno del campo parent e depth, per ora li lascio