//*()!&
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stack>
#include <queue>

using namespace std;

typedef unsigned long long ull;

#define MAX 200

/*
typedef struct a{
  string nome;
  string atributo;
  string velho;
  string novo;
} transaction_t;
*/

class operation {
  public:
    string nome;
    string atributo;
    string velho;
    string novo;

    operation(string no, string atr, string vel, string nov){
      nome = no;
      atributo = atr;
      velho = vel;
      novo = nov;
    }
};

class transaction {
  public:
    string nome;
    bool committed;
    bool started;

    transaction(string n, bool com, bool sta){
      nome = n;
      committed = com;
      started = sta;
    }
};


vector <transaction> transacoes;
map <string, string> banco;
stack <operation> redo;
queue <operation> undo;

//void printarOperacoes();
//void printarTransacoes();
void carregarBanco();
void printarBanco();
void analisarLog();
vector <transaction>::iterator find(string comparator);
void redu();
void undu();
int main(void){
  // transaction t(string("T1"), string("A"), string("1"), string("2"));
  // printf("%s %s %s %s\n", t.nome.c_str(), t.atributo.c_str(), t.velho.c_str(), t.novo.c_str());
  carregarBanco();
  printf("Dados do banco antes da recuperação:\n");
  printarBanco();
  analisarLog();
  undu();
  redu();
  printf("\nDados do banco depois da recuperação:\n");
  printarBanco();
  //printarTransacoes();
  //printarOperacoes();
  return 0;
}

void carregarBanco(){
  FILE *f;
  int i;
  char c;
  char atributo[MAX];
  char valor[MAX];

  f = fopen("entrada.txt", "r");                                  // Abre o arquivo
  if(f == NULL){
    printf("Azedô\n");
    return;
  }
  while((c = fgetc(f)) != EOF){                                   // Enquanto nao terminar o arquivo
    for(i = 0; c != '=' && c != EOF; i++){
      atributo[i] = c;
      c = fgetc(f);
    }
    atributo[i] = '\0';
    for(i = 0; c != '\n' && c != EOF; i++){                       // Enquanto ele nao chegar na nova linha
      c = fgetc(f);                                               // ou acabar o arquivo
      valor[i] = c;
    }
    valor[i-1] = '\0';
    if(c != EOF) banco[atributo] = valor;                         // Se não for EOF, joga no map.
  }
  fclose(f);                                                      // Fecha o arquivo
  return;
}

void printarBanco(){                                              // itera no map e printa ele

  printf("\n##BANCO##\n");
  for(map<string, string>::iterator i = banco.begin(); i != banco.end(); i++){
    printf("%s = %s\n", i->first.c_str(), i->second.c_str());
  }
  printf("\n");
}

void analisarLog(){

  FILE *f;
  char c, s[MAX], junk[MAX], tmpname[MAX];
  char tmpname2[MAX], tmpname3[MAX], tmpname4[MAX];
  ull i, j, k, size;
  string finder;
  int countUndo = 0;                                            // Quantia de undo-trans. esperando pelo seu start
  vector<transaction>::iterator aux;                            // Iterador auxiliar para busca
  bool ckptFound = false;                                       // Flag para verificar se passou por um end ckpt
  bool terminator = false;                                      // Flag se ele deve terminar de avaliar
  bool passouSc = false;                                        // Flag se ja passou do start checkpoint

  f = fopen("log.txt", "r");
  fseek(f, 0, SEEK_END);
  size = ftell(f);                                              // salva o valor do tamanho do arquivo
  for(i = 2; i <= size && (!terminator); i++){                      // Vai iterando até chegar no começo do arquivo
    fseek(f, size-i, SEEK_SET);
    if((c = fgetc(f)) == '\n' || i == size){                        // Se é /n, é pq iniciou uma nova operação
      if(i == size) fseek(f, 0, SEEK_SET);                          // gambiarra pra pregar primeira linha
      fgets(s, MAX, f);                                             // Pega essa operação.
      strcpy(junk, s);                                           // Copia para uma variavel que possa ser zoada
      junk[strlen(junk)-1] = '\0';                               // Coloca o /0 nela
//                  END CHECKPOINT CASE
      if(!strcmp(junk, "<END CKPT>")){ ckptFound = true; /*printf("end ckpt\n");*/}             // Se a operação for END_CKPT...
//                  START CHECKPOINT CASE
      else if(junk[11] = '\0', !strcmp(junk, "<Start CKPT")){    // Se a operação for start checkpoint...
        //printf("start ckpt\n");
        if(ckptFound && !passouSc){
          for(j = 12, k = 0; s[j] != ')';){                        // Ler os nomes das transações
            while(s[j] != ',' && s[j] != ')'){
              tmpname[k++] = s[j++];
            }
            tmpname[k] = '\0';
            if(s[j] != ')') j++;
            k = 0;
            finder = tmpname;
            aux = find(tmpname);
            if(aux == transacoes.end()){
                countUndo++;                                                      // Se a transação nao existir ou nao for
                transacoes.push_back(transaction(tmpname, false, false));         // Comitada, aumentar o count de undo e por no vector
              }
            }
          passouSc = true;
          if(!countUndo) terminator = true;            // Se nao houver transações esperando starts, Terminar análise
          /*printf("##FLAGS##\n");
          printf("ckptFound: %s\n", ckptFound ? "true" : "false");
          printf("terminator: %s\n", terminator ? "true" : "false");
          printf("passouSc: %s\n", passouSc ? "true" : "false");
          printf("%d\n", countUndo);*/
        }
      }
//                  COMMIT CASE
      else if(junk[7] = '\0', !strcmp(junk, "<Commit")){           // Se for um commit...
        //printf("commit\n");
        for(j = 8, k = 0; s[j] != '>'; j++) tmpname[k++] = s[j];
        tmpname[k] = '\0';
        if(!passouSc)
          transacoes.push_back(transaction(tmpname, true, false));
      }
//                  START TRANSACTION CASE
      else if(junk[6] = '\0', !strcmp(junk, "<start")){            // Se for start transação...
        //printf("start transaction\n");
        for(j = 7, k = 0; s[j] != '>'; j++) tmpname[k++] = s[j];
        tmpname[k] = '\0';
        aux = find(tmpname);
        if(aux != transacoes.end()){
          aux->started = true;
          if(!aux->committed) countUndo--;
        }
        if(!countUndo && passouSc) terminator = true;            // Se nao houver transações esperando starts,
                                                                  // Terminar análise
      }
//                  OPERATION CASE
      else{                                                     // Se for uma operação de modificação no banco...
        for(j = 1, k = 0; s[j] != ','; j++, k++) tmpname[k] = s[j]; tmpname[k] = '\0';
        for(j++, k = 0; s[j] != ','; j++, k++) tmpname2[k] = s[j]; tmpname2[k] = '\0';
        for(j++, k = 0; s[j] != ','; j++, k++) tmpname3[k] = s[j]; tmpname3[k] = '\0';
        for(j++, k = 0; s[j] != '>'; j++, k++) tmpname4[k] = s[j]; tmpname4[k] = '\0';
        aux = find(string(tmpname));
        if(aux == transacoes.end() && !passouSc){
          transacoes.push_back(transaction(tmpname, false, false));
          undo.push(operation(tmpname, tmpname2, tmpname3, tmpname4));
          countUndo++;
        }
        else if(!aux->committed) undo.push(operation(tmpname, tmpname2, tmpname3, tmpname4));
        else if(!passouSc) redo.push(operation(tmpname, tmpname2, tmpname3, tmpname4));
      }
    }
  }
}

vector <transaction>::iterator find(string comparator){
  vector<transaction>::iterator j;
  for(j = transacoes.begin(); j != transacoes.end(); j++){
    if(comparator == j->nome){
      break;
    }
  }
  return j;
}

void printarTransacoes(){
  vector<transaction>::iterator j;
  printf("##TRANSAÇÕES##\n");
  for(j = transacoes.begin(); j != transacoes.end(); j++){
      printf("Nome: %s\nCommited: %s\nStarted: %s\n", j->nome.c_str(), j->committed ? "true" : "false", j->started ? "true" : "false");
  }
}

// void printarOperacoes(){
//   operation j("", "", "", "");
//   printf("##OPERAÇÕES##\n");
//   printf("##Redo##\n");
//   while(!redo.empty()){
//     j = redo.top();
//     redo.pop();
//     printf("<%s,%s,%s,%s>\n", j.nome.c_str(), j.atributo.c_str(), j.velho.c_str(), j.novo.c_str());
//   }
//   printf("##Undo##\n");
//   while(!undo.empty()){
//     j = undo.front();
//     undo.pop();
//     printf("<%s,%s,%s,%s>\n", j.nome.c_str(), j.atributo.c_str(), j.velho.c_str(), j.novo.c_str());
//   }
// }

void undu(){
 operation op("", "", "", "");
 while(!undo.empty()){
   op = undo.front();
   undo.pop();
   banco[op.atributo] = op.velho;
   printf("Operação sendo desfeita: <%s,%s,%s,%s>\n", op.nome.c_str(), op.atributo.c_str(), op.velho.c_str(), op.novo.c_str());
 }
}

void redu(){
  operation op("", "", "", "");
  while(!redo.empty()){
    op = redo.top();
    redo.pop();
    banco[op.atributo] = op.novo;
    printf("Operação sendo refeita: <%s,%s,%s,%s>\n", op.nome.c_str(), op.atributo.c_str(), op.velho.c_str(), op.novo.c_str());
  }
}
