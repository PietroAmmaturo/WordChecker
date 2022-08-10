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
	char discoveredOccurrences[ALPH_LEN];

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

	// numero di lettere che appaiono almeno una volta
	int numberOfDiscoveredOccurrences;
} info_t;

// come strutturo un nodo dell'albero rappresentante il dizionario
typedef struct tree_s
{
	// profondità a cui si trova un nodo
	short unsigned int depth : 4;
	// carattere contenuto nel nodo
	char character;
	char ggg;

	// se valido (da stampare)
	_Bool valid;
	// figli del nodo (una lista dinamica)
	struct list_s *child;
} tree_t;

typedef struct tree2_s
{
	// carattere contenuto nel nodo
	// profondità a cui si trova un nodo
	short unsigned int caracter : 2;
	// se valido (da stampare)
	_Bool valid;
	// figli del nodo (una lista dinamica)
	struct list_s *child;
} tree2_t;

// come strutturo un nodo della lista rappresentante i figli di ogni nodo
typedef struct list_s
{
	// puntatore al nodo
	tree_t *node;
	// puntatore al prossimo fratello
	struct list_s *nextSibiling;
} list_t;

int depth;

// dichiarazioni

int characterToIndex(char c);

list_t *findChild(list_t *h, char c);

_Bool findWord(tree_t *head, char *word, int length);

tree_t *addNode(tree_t *h, char c, int d);

list_t *addChildBetween(list_t *last, list_t *next, char c, int d);

list_t *findOrAddChild(list_t *h, char c, int d, list_t **found);

tree_t *addWord(tree_t *head, char *word, int length);

tree_t *addWords(tree_t *head, char *word, int length);

void printDictionary(tree_t *h, char *word, int length);

void cutBranch(tree_t *h);

void lowerCounters(tree_t *h, int *counter);

void validBranch(tree_t *h);

void resetFiltered(tree_t *h);

void filterDictionary(info_t *info, tree_t *h, int *counter, int *lettersWithEnoughOccurrences);

void startMatch(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length);

void compareWords(info_t *info, char *word, char *solution, char *result, _Bool *isFree, int length, int *counter);

//////////////
//	utils	//
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
_Bool findWord(tree_t *head, char *word, int length)
{
	int i;
	tree_t *treeCursor;
	list_t *rightChild;

	treeCursor = head;

	// scorro la parola inserita
	for (i = 0; i < length; i++)
	{
		// trovo il figlio, devo pure aggiornare l'head nel caso cambi
		rightChild = findChild(treeCursor->child, word[i]);
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
//	alberi	//
//////////////
tree_t *addNode(tree_t *h, char c, int d)
{
	// adding node
	h = (tree_t *)malloc(sizeof(tree_t));
	if (h)
	{
		h->character = c;
		h->depth = d;
		h->valid = 1;
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
		treeCursor->child = findOrAddChild(treeCursor->child, word[i], i, &rightChild);
		// sposto il cursore nel figlio (che sia stato appena aggiunto o meno è indifferente)
		treeCursor = rightChild->node;
	}
	return head;
}
tree_t *addWords(tree_t *head, char *word, int length)
{
	// printf("\n----inizio inserimento----\n");
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
	// printf("\n----fine inserimento----\n");

	return head;
}
// rende valido l'elemento che gli viene passato e tutti i suoi figli
void validBranch(tree_t *h)
{
	list_t *listCursor;

	// All the children
	for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
		validBranch(listCursor->node);

	h->valid = 1;
}

//////////////
//	liste	//
//////////////

// unica funzione che aggiunge il figlio se non c'è
// oppure ritorna l'indirizzo del figlio se c'è
list_t *findOrAddChild(list_t *h, char c, int d, list_t **found)
{
	list_t *cursor, *previous;
	previous = NULL;
	// sposto il cursore fino a trovare il figlio che punta a un nodo il cui carattere è maggiore o uguale di quello desiderato
	for (cursor = h; cursor != NULL && cursor->node->character < c; cursor = cursor->nextSibiling)
		previous = cursor;
	// se non lo trovo lo aggiungo
	if (cursor == NULL || cursor->node->character > c)
	{
		cursor = addChildBetween(previous, cursor, c, d);
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
list_t *addChildBetween(list_t *last, list_t *next, char c, int d)
{
	list_t *newChild;

	newChild = (list_t *)malloc(sizeof(list_t));
	// adding node
	if (newChild)
	{
		newChild->nextSibiling = next;
		newChild->node = addNode(newChild->node, c, d);
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
list_t *findChild(list_t *h, char c)
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
////////////////////
//	CLOSE TO I/O  //
////////////////////
void filterDictionary(info_t *info, tree_t *h, int *counter, int *lettersWithEnoughOccurrences)
{
	list_t *listCursor;
	int characterIndex;

	// controllo di non essere nell'head
	if (h->character != EOS)
	{
		depth++;
		characterIndex = characterToIndex(h->character);
		// conto i caratteri
		counter[characterIndex]++;
		// se il carattere non si può trovare a quella posizione (profondità)
		// printf("\nchecking position of: %c at: %d, result: %d\n", h->character, h->depth, info->isPositionOfCharacterValid[h->depth][characterToIndex(h->character)]);
		if (!(info->isPositionOfCharacterValid[depth][characterIndex]))
		{
			// printf("\nwrong position of: %c at: %d\n", h->character, h->depth);
			//    rendo non valido il branch dalla radice morta in giu e decremento i contatori
			h->valid = 0;
			counter[characterIndex]--;
			depth--;
			return;
		}
		// se il numero di una lettera supera il numero effettivo di occorrenze (e si è certi del numero)
		if (counter[characterIndex] > info->discoveredOccurrences[characterIndex] && info->isDefinitive[characterIndex])
		{
			// printf("\nto many of: %c\n", h->character);
			//   rendo non valido il branch dalla radice morta in su e decremento i contatori
			h->valid = 0;
			counter[characterIndex]--;
			depth--;
			return;
		}
		if (counter[characterIndex] == info->discoveredOccurrences[characterIndex])
		{
			(*lettersWithEnoughOccurrences)++;
		}
		// quando raggiungo la fine della parola
		if (h->child == NULL)
		{
			if ((*lettersWithEnoughOccurrences) == info->numberOfDiscoveredOccurrences)
			{
			}
			else
			{
				// rendo non valido il branch dalla radice morta in su e decremento i contatori
				h->valid = 0;
			}
		}
		// All the valid children
		for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
			if (listCursor->node->valid)
				filterDictionary(info, listCursor->node, counter, lettersWithEnoughOccurrences);
		;
		// se il contatore della lettera, che mi garantiva che ci fossero abbastanza
		// occorrenze della lettera stessa, va sotto la soglia, non ci sono più abbastanza occorrenze
		if (counter[characterIndex] == info->discoveredOccurrences[characterIndex])
		{
			(*lettersWithEnoughOccurrences)--;
		}
		// al ritorno decremento la lettera
		counter[characterIndex]--;
		depth--;
	}

	else
	{
		int i;
		// inizializzo il contatore di caratteri a zero
		for (i = 0; i < ALPH_LEN; i++)
		{
			counter[i] = 0;
		}
		// inizializzo il numero di lettere che hanno abbastanza occorrenze
		(*lettersWithEnoughOccurrences) = 0;
		// filtro il dizionario aggiornando i validi
		depth = -1;
		for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
			if (listCursor->node->valid)
				filterDictionary(info, listCursor->node, counter, lettersWithEnoughOccurrences);
	}
}
void countFiltered(tree_t *h, int *result)
{
	list_t *listCursor;

	if (h->child == NULL)
		(*result)++;
	else
		// All the children
		for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
			if (listCursor->node->valid)
				countFiltered(listCursor->node, result);
}

//////////////
//	I/O	    //
//////////////
void printDictionary(tree_t *h, char *word, int length)
{
	list_t *listCursor;

	// posiziona il carattere nella parola contenitore
	word[h->depth] = h->character;

	// printf("%c, %d\n", h->character, h->valid);

	// stampa la parola quando raggiungi la fine
	if (h->child == NULL)
		printf("%.*s\n", length, word);
	else
		// All the children
		for (listCursor = h->child; listCursor != NULL; listCursor = listCursor->nextSibiling)
			if (listCursor->node->valid)
				printDictionary(listCursor->node, word, length);
}
void compareWords(info_t *info, char *word, char *solution, char *result, _Bool *isFree, int length, int *counter)
{
	int i, j, found, definitive, characterIndex;

	for (i = 0; i < length; i++)
	{
		characterIndex = characterToIndex(word[i]);
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
			info->isPositionOfCharacterValid[i][characterIndex] = 1;
			// conto la lettera
			counter[characterIndex]++;
		}
		else
		{
			// per ora non è stata trovata
			result[i] = '/';
			// non è associata
			isFree[i] = 1;
			// i caratteri che non sono risultati giusti al posto giusto li rimuovo
			info->isPositionOfCharacterValid[i][characterIndex] = 0;
		}
	}
	for (i = 0; i < length; i++)
	{
		characterIndex = characterToIndex(word[i]);
		definitive = 0;
		found = 0;
		for (j = 0; j < length && !found; j++)
		{
			// controllo se esiste una uguale non associata e non fissata (ancora '/' e non '+')
			if (word[i] == solution[j] && result[i] != '+')
			{
				// se trovo una non associata
				if (isFree[j])
				{
					// trovata, devo passare alla prossima lettera
					// anche per non contare le altre
					found = 1;
					// giusta al posto sbagliato
					result[i] = '|';
					// conto le lettere
					counter[characterIndex]++;
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
			info->discoveredOccurrences[characterIndex] = info->trueOccurrences[characterIndex];
			info->isDefinitive[characterIndex] = 1;
		}
		// controllo se viene trovata
		found = 0;
		for (j = 0; j < length && !found; j++)
		{
			// controllo se viene trovata
			if (word[i] == solution[j])
			{
				// segno che è stata trovata e posso uscire dal ciclo
				found = 1;
			}
		}
		// se questo è vero, è proprio sbagliata!!, la rimuovo in toto (da ogni posizione)
		if (!found)
		{
			for (j = 0; j < length; j++)
			{
				info->isPositionOfCharacterValid[j][characterIndex] = 0;
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
			if (counter[i] > info->discoveredOccurrences[i])
			{
				info->discoveredOccurrences[i] = counter[i];
			}
		}
		// aggiorno il numero di occorrenze scoperte
		if (info->discoveredOccurrences[i] > 0)
		{
			info->numberOfDiscoveredOccurrences++;
		}
	}
	// stampa risultato del confronto
	printf("%.*s\n", length, result);

	// mostro caratteri che posso trovare alle posizioni
	/*
		for (i = 0; i < length; i++)
		{
			for (j = 0; j < ALPH_LEN; j++)
				printf("-%d-", info->isPositionOfCharacterValid[i][j]);
			printf("\n");
		}
		printf("\n");
		printf("\ncounter\n");
		for (i = 0; i < ALPH_LEN; i++)
		{
			printf("%d", counter[i]);
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
void startMatch(info_t *info, tree_t *head, char *word, char *solution, char *result, _Bool *isFree, int length)
{
	int i, j, attempts, enoughOccurrences;
	char character;
	char command[MAX_COMMAND_LENGTH];
	int charactersCounter[ALPH_LEN], filteredCounter;

	//  prendo la soluzione
	for (i = 0; i < length; i++)
		solution[i] = getc(stdin);
	// prendo il null
	character = getc(stdin);
	// scan per sapere quanti tentativi ho a disposizione in questa partita
	if (scanf("%d", &attempts))
		;
	//  prendi primo carattere
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
	// inizializzo le occorrenze scoperte
	info->numberOfDiscoveredOccurrences = 0;

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
				printDictionary(head, word, length);
			}
			// comando per aggiungere parole al dizionario durante una partita
			else if (strcmp(command, "inserisci_inizio\n") == 0)
			{
				head = addWords(head, word, length);
				// devo "buttare" la parte finale di comando ("inserisci fine")
				if (fgets(command, MAX_COMMAND_LENGTH, stdin))
					;
				filterDictionary(info, head, charactersCounter, &enoughOccurrences);
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
					printf("ok\n");
					validBranch(head);
					return;
				}
				// inizializzo il contatore delle filtrate a zero
				filteredCounter = 0;
				// inizializzo il contatore di caratteri a zero
				for (j = 0; j < ALPH_LEN; j++)
				{
					charactersCounter[j] = 0;
				}
				// inizializzo i free
				for (j = 0; j < length; j++)
					isFree[i] = 0;
				// paragono la parola alla soluzione, stampo il risultato del confronto e aggiorno info
				compareWords(info, word, solution, result, isFree, length, charactersCounter);

				filterDictionary(info, head, charactersCounter, &enoughOccurrences);
				// stampa numero di parole rimaste
				countFiltered(head, &filteredCounter);
				printf("%d\n", filteredCounter);
			}
			else
			{
				printf("not_exists\n");
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
		printf("ko\n");
		validBranch(head);
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
	/*
		printf("%ld\n", sizeof(tree_t));
		printf("%ld\n", sizeof(tree2_t));
		printf("%ld\n", sizeof(list_t));
		printf("%ld\n", sizeof(_Bool));
	*/
	// scan per sapere quanto è lunga la parola
	if (scanf("%d", &wordLength))
		;
	// creo dizionario vuoto
	dictionary = addNode(dictionary, '\0', (wordLength - 1));

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
				startMatch(&infoVar, dictionary, word, solution, result, isFree, wordLength);
			}
			// comando per aggiungere parole al dizionario tra una partita e l'altra
			else if (strcmp(command, "inserisci_inizio\n") == 0)
			{
				dictionary = addWords(dictionary, word, wordLength);
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

// si può fare probabilemnte a meno del campo parent e depth, per ora li lascio

// c'è un problema con il riconoscimento delle giuste al posto sbagliato!
// oppure con il contatore delle occorrenze
// problema con il contatore di occorrenze, il decremento forse è sbagliato come idea
// idea: contarle dal basso verso l'alto
// non decrementarle più con findRootOfDeadBranch