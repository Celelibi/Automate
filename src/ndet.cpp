#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <list>
#include <cassert>
#include <utility>

// pour la seconde partie du projet
#include "expression_rationnelle.hpp"
#include "parser.hpp"

using namespace std;

////////////////////////////////////////////////////////////////////////////////

const unsigned int ASCII_A = 'a';
const unsigned int ASCII_Z = 'z';
const bool         DEBUG = false;

typedef size_t                            etat_t;
typedef unsigned char                     symb_t;
typedef set< etat_t >                     etatset_t;
typedef vector< etatset_t >               transdest_t;
typedef vector< transdest_t >             trans_t;
typedef vector< etatset_t >               epsilon_t;
typedef map< etatset_t, etat_t >          map_t;

////////////////////////////////////////////////////////////////////////////////

struct sAutoNDE
{
  // caractéristiques
  size_t nb_etats;
  size_t nb_symbs;
  size_t nb_finaux;

  etat_t initial;
  // état initial

  etatset_t finaux;
  // états finaux : finaux_t peut être un int*, un tableau dynamique comme
  // vector<int> ou une autre structure de donnée de votre choix.

  trans_t trans;
  // matrice de transition : trans_t peut être un int***, une structure
  // dynamique 3D comme vector< vector< set<int> > > ou une autre structure de
  // donnée de votre choix.

  epsilon_t epsilon;
  // transitions spontanées : epsilon_t peut être un int**, une structure
  // dynamique 2D comme vector< set<int> > ou une autre structure de donnée de
  // votre choix.
};

////////////////////////////////////////////////////////////////////////////////

bool FromFile(sAutoNDE &at, string path)
{
  ifstream myfile(path.c_str(), ios::in);
  //un flux d'entree obtenu à partir du nom du fichier
  string line;
  // un ligne lue dans le fichier avec getline(myfile,line);
  istringstream iss;
  // flux associé à la chaine, pour lire morceau par morceau avec >> (comme cin)
  etat_t s(0), t(0);
  // deux états temporaires
  symb_t a(0);
  // un symbole temporaire

  if (myfile.is_open())
  {
    // la première ligne donne 'nb_etats nb_symbs nb_finaux'
    do
    {
      getline(myfile, line);
    } while (line.empty() || line[0] == '#');
    // on autorise les lignes de commentaires : celles qui commencent par '#'
    iss.str(line);
    if ((iss >> at.nb_etats).fail() ||
        (iss >> at.nb_symbs).fail() ||
        (iss >> at.nb_finaux).fail())
    {
      return false;
    }
    // la deuxième ligne donne l'état initial
    do
    {
      getline (myfile, line);
    } while (line.empty() || line[0] == '#');
    iss.clear();
    iss.str(line);
    if ((iss >> at.initial).fail())
    {
      return -1;
    }

    // les autres lignes donnent les états finaux
    for (size_t i = 0; i < at.nb_finaux; i++)
    {
      do
      {
        getline (myfile, line);
      } while (line.empty() || line[0] == '#');
      iss.clear();
      iss.str(line);
      if ((iss >> s).fail())
      {
        continue;
      }
      //        cerr << "s= " << s << endl;
      at.finaux.insert(s);
    }

    // on alloue les vectors à la taille connue à l'avance pour éviter les
    // resize dynamiques
    at.epsilon.resize(at.nb_etats);
    at.trans.resize(at.nb_etats);
    for (size_t i = 0; i < at.nb_etats; ++i)
    {
      at.trans[i].resize(at.nb_symbs);
    }

    // lecture de la relation de transition
    while (myfile.good())
    {
      line.clear();
      getline(myfile, line);
      if (line.empty() && line[0] == '#')
      {
        continue;
      }
      iss.clear();
      iss.str(line);

      // si une des trois lectures échoue, on passe à la suite
      if ((iss >> s).fail() ||
          (iss >> a).fail() ||
          (iss >> t).fail() ||
          (a < ASCII_A ) ||
          (a > ASCII_Z ))
      {
        continue;
      }

      //test epsilon ou non
      if ((a - ASCII_A) >= at.nb_symbs)
      {
        //        cerr << "s=" << s<< ", (e), t=" << t << endl;
        at.epsilon[s].insert(t);
      }
      else
      {
        // cerr << "s=" << s<< ", a=" << a-ASCII_A << ", t=" << t << endl;
        at.trans[s][a - ASCII_A].insert(t);
      }
    }
    myfile.close();
    return true;
  }
  return false;
  // on ne peut pas ouvrir le fichier
}


// -----------------------------------------------------------------------------
// Fonctions à compléter pour la première partie du projet
// -----------------------------------------------------------------------------


bool EstDeterministe(const sAutoNDE &at)
{
  unsigned int it, it2;
  for (it = 0; it < at.nb_etats; it++)
  {
    // s'il y a des transitions epsilon = pas déterministe
    if (at.epsilon[it].size() != 0)
    {
      return false;
    }

    // pour chaque états, il ne doit pas y avoir plus d'une transition pour un
    // symbole donné.
    for (it2 = 0; it2 < at.nb_symbs; it2++)
    {
      if (at.trans[it][it2].size() > 1)
      {
        return false;
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void Fermeture(const sAutoNDE &at, etatset_t &e)
{
  // Cette fonction clôt l'ensemble d'états E={e_0, e_1, ... ,e_n} passé en
  // paramètre avec les epsilon transitions
  etatset_t f;
  etatset_t::const_iterator it, it2;

  // pour chaque états, on renvoie un ensemble d'états accessible via des
  // epsilon transition.
  do
  {
    f = e;
    for (it = f.begin(); it != f.end(); it++)
    {
      for (it2 = at.epsilon[*it].begin(); it2 != at.epsilon[*it].end(); it2++)
      {
        e.insert(*it2);
      }
    }
  } while (e.size() != f.size());
}

////////////////////////////////////////////////////////////////////////////////

etatset_t Delta(const sAutoNDE &at, const etatset_t &e, symb_t c)
{
  etatset_t res, tmp;
  etatset_t::const_iterator it, it2;

  tmp = e;
  Fermeture(at, tmp);

  // pour chaque états, on renvoie un ensemble d'états accessible avec le
  // symbole c
  for (it = tmp.begin(); it != tmp.end(); it++)
  {
    for (it2 = at.trans[*it][c - ASCII_A].begin();
        it2 != at.trans[*it][c - ASCII_A].end();
        it2++)
    {
      res.insert(*it2);
    }
  }

  Fermeture(at, res);
  return res;
}

////////////////////////////////////////////////////////////////////////////////

template < class T >
bool setHasIntersection(const set< T > &s1, const set< T > &s2)
{
  vector< T > in(s1.size());
  typename vector< T >::iterator it;

  it = set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), in.begin());
  return (it != in.begin());
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Determinize(const sAutoNDE &at)
{
  sAutoNDE r;
  symb_t c;
  map_t combstate;
  vector< etatset_t > statecomb;
  queue< etat_t > q;
  etat_t current;
  map_t::const_iterator mit;
  etatset_t::const_iterator sit;

  // cas simple, s'il est déjà déterministe, on le renvoie tel quel
  if (EstDeterministe(at))
  {
    return at;
  }

  r.nb_etats = 0;
  r.nb_symbs = at.nb_symbs;
  r.nb_finaux = 0;
  r.initial = 0;

  // créer l'état initial déterministe.
  r.nb_etats++;
  r.trans.resize(r.nb_etats);
  r.trans[r.initial].resize(r.nb_symbs);
  statecomb.resize(r.nb_etats);
  statecomb[r.initial].insert(at.initial);
  Fermeture(at, statecomb[r.initial]);
  combstate[statecomb[r.initial]] = r.initial;

  if (setHasIntersection(at.finaux, statecomb[r.initial]))
  {
    r.finaux.insert(r.initial);
  }

  q.push(r.initial);

  while (!q.empty())
  {
    current = q.front();
    q.pop();
    for (c = ASCII_A; c < ASCII_A + r.nb_symbs; c++)
    {
      // récupère l'ensemble des états nondet accessible par le symbole c à
      // partir de current.
      etatset_t tmp = Delta(at, statecomb[current], c);
      if (tmp.empty())
      {
        continue;
      }
      // créer un nouvel état et met à jour la correspondance entre les
      // combinaison d'états nondet vers un état det.
      if (combstate.find(tmp) == combstate.end())
      {
        etat_t newstate = r.nb_etats++;
        r.trans.resize(r.nb_etats);
        r.epsilon.resize(r.nb_etats);
        r.trans[newstate].resize(r.nb_symbs);

        statecomb.resize(r.nb_etats);
        statecomb[newstate] = tmp;
        combstate[tmp] = newstate;

        // Il faudra ajouter toutes les transitions qui partent de ce nouvel
        // état.
        q.push(newstate);

        // Ce nouvel état est final si l'un des états de l'automate
        // non-déterministe auxquels il correspond est final.
        if (setHasIntersection(at.finaux, tmp))
        {
          r.finaux.insert(newstate);
        }
      }
      r.trans[current][c - ASCII_A].insert(combstate[tmp]);
    }
  }

  r.nb_finaux = r.finaux.size();

  return r;
}

////////////////////////////////////////////////////////////////////////////////

bool Accept(const sAutoNDE &at, string str)
{
  sAutoNDE r;
  string::const_iterator it;
  etat_t current;

  r = Determinize(at);

  current = r.initial;
  // pour chaque caractère du mot :
  for (it = str.begin(); it != str.end(); ++it)
  {
    // si le caractère courant n'est pas dans l'alphabet.
    if ((symb_t)*it < ASCII_A || (symb_t)*it >= ASCII_A + r.nb_symbs)
    {
      return false;
    }
    // s'il n'existe pas de transition depuis l'état courant avec le symbole
    // courant, il ne peut pas y avoir plus d'une transition car on a
    // déterminiser l'automate.
    etatset_t &tmp = r.trans[current][*it - ASCII_A];
    assert(tmp.size() <= 1);
    if (tmp.size() != 1)
    {
      return false;
    }
    current = *tmp.begin();
  }

  return r.finaux.find(current) != r.finaux.end();
}

////////////////////////////////////////////////////////////////////////////////

ostream &operator<<(ostream &out, const sAutoNDE &at)
{
  etatset_t::const_iterator sit;
  unsigned int it, it2;

  // <nb_etats> <nb_symboles> <nb_etats_finaux>
  out << at.nb_etats << " " << at.nb_symbs << " " << at.nb_finaux << endl;
  // <état initial>
  out << at.initial << endl;
  // pour chaque états finaux, on l'ajoute à la suite <état final n>
  for (sit = at.finaux.begin(); sit != at.finaux.end(); ++sit)
  {
    out << *sit << endl;
  }
  // pour chaque états, on écrit <état x> <symbole y> <état z>
  for (it = 0; it < at.nb_etats; it++)
  {
    for (it2 = 0; it2 < at.nb_symbs; it2++)
    {
      for (sit = at.trans[it][it2].begin();
          sit != at.trans[it][it2].end();
          ++sit)
      {
        out << it << " " << (char)(it2 + ASCII_A) << " " << *sit << endl;
      }
    }
  }

  return out;
}

////////////////////////////////////////////////////////////////////////////////

bool ToGraph(sAutoNDE &at, string path)
{
  etatset_t::const_iterator sit;
  unsigned int it, it2;
  ofstream out(path.c_str(), ios::out);
  if(!out){ //verification s'il a bien ete ouvert
      return false;
  }
  out << "digraph finite_state_machine {" << endl;
  out << "\trankdir=LR;" << endl;
  out << "\tsize=\"10,10\"" << endl << endl;
  // tous les états finaux représenté par des double cercle.
  out << "\tnode [shape = doublecircle]; ";
  for (sit = at.finaux.begin(); sit != at.finaux.end(); ++sit)
  {
    out << *sit << " ";
  }
  out << ";" << endl;
  // l'etat initiale representé par un point et une fleche.
  out << "\tnode [shape = point ]; q;" << endl;
  // les etats sont representé par des cercles
  out << "\tnode [shape = circle];" << endl << endl;
  out << "\tq -> " << at.initial << ";" << endl;
  // <état départ> -> <état final> [label = "<char>"];
  // toutes les transitions non spontanée
  for (it = 0; it < at.nb_etats; it++)
  {
    for (it2 = 0; it2 < at.nb_symbs; it2++)
    {
      for (sit = at.trans[it][it2].begin();
          sit != at.trans[it][it2].end();
          ++sit)
      {
        out << "\t" << it << " -> " << *sit;
        out << " [label = \"" << (char)(it2 + ASCII_A) << "\"];" << endl;
      }
      for (sit = at.epsilon[it].begin();
          sit != at.epsilon[it].end();
          ++sit)
      {
        out << "\t" << it << " -> " << *sit;
        out << " [label = \"ε\"];" << endl;
      }
    }
  }

  out << endl << "}" << endl;
  return true;
}


// -----------------------------------------------------------------------------
// Fonctions à compléter pour la seconde partie du projet
// -----------------------------------------------------------------------------

sAutoNDE Append(const sAutoNDE &x, const sAutoNDE &y)
{
  // fonction outil : on garde x, et on "ajoute" trans et epsilon de y
  // en renommant ses états, id est en décalant les indices des états de y
  // de x.nb_etats
  assert(x.nb_symbs == y.nb_symbs);
  sAutoNDE r;

  //TODO définir cette fonction

  return r;
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Union(const sAutoNDE &x, const sAutoNDE &y)
{
  assert(x.nb_symbs == y.nb_symbs);
  sAutoNDE r = Append(x, y);

  //TODO définir cette fonction

  return r;
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Concat(const sAutoNDE &x, const sAutoNDE &y)
{
  assert(x.nb_symbs == y.nb_symbs);
  sAutoNDE r = Append(x, y);

  //TODO définir cette fonction

  return r;
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Complement(const sAutoNDE &x)
{
  //TODO définir cette fonction

  return x;
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Kleene(const sAutoNDE &x)
{
  //TODO définir cette fonction

  return x;
}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE Intersection(const sAutoNDE &x, const sAutoNDE &y)
{
  //TODO définir cette fonction

  return x;

}

////////////////////////////////////////////////////////////////////////////////

sAutoNDE ExpressionRationnelle2Automate(string expr)
{
  cout << "Construction d'un automate à partir d'une expression rationnelle\n";
  cout << "  Expression en entrée (string) : " << expr << endl;

  sExpressionRationnelle er = lit_expression_rationnelle(expr);

  cout << "  Expression en entrée (ASA)    : " << er << endl;

  sAutoNDE r;

  //TODO définir cette fonction

  return r;
}

////////////////////////////////////////////////////////////////////////////////

string Automate2ExpressionRationnelle(sAutoNDE at)
{
  cout << "Construction d'une expression rationnelle à partir d'un automate\n";

  string sr;

  //TODO définir cette fonction

  return sr;
}

////////////////////////////////////////////////////////////////////////////////

bool Equivalent(const sAutoNDE &a1, const sAutoNDE &a2)
{

  //TODO définir cette fonction

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void Help(ostream &out, char *s)
{
  out << "Utilisation du programme " << s << " :" << endl ;
  out << "-acc ou -accept Input Word :\n\t détermine si le mot Word est ";
  out << "accepté par l'automate Input" << endl;
  out << "-det ou -determinize Input Output [-g] :\n\t déterminise l'automate ";
  out << "Input, écrit le résultat dans Output" << endl;
  out << "-isdet ou -is_deterministic Input :\n\t détermine si l'automate ";
  out << "Input est déterministe" << endl;
  out << "-aut2expr ou automate2expressionrationnelle Input :\n\t calcule ";
  out << "l'expression rationnelle correspondant à l'automate Input et ";
  out << "l'affiche sur la sortie standard" << endl;
  out << "-expr2aut ou expressionrationnelle2automate ExpressionRationnelle ";
  out << "Output [-g] :\n\t calcule l'automate correspondant à ";
  out << "ExpressionRationnelle, écrit l'automate résultant dans Output";
  out << endl;
  out << "-equ ou -equivalent Input1 Intput2 :\n\t détermine si les deux ";
  out << "automates Input1 et Input2 sont équivalents" << endl;
  out << "-nop ou -no_operation Input Output [-g] :\n\t ne fait rien de ";
  out << "particulier, recopie l'entrée dans Output" << endl;

  out << "Exemple '" << s << " -determinize auto.txt resultat -g" << endl;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[] )
{
  if (argc < 3)
  {
    Help(cout, argv[0]);
    return EXIT_FAILURE;
  }

  int pos;
  int act = -1;               // pos et act pour savoir quelle action effectuer
  int nb_ifiles = 0;          // nombre de fichiers en entrée
  int nb_ofiles = 0;          // nombre de fichiers en sortie
  string str, in1, in2, out, acc, expr;
  // chaines pour (resp.) tampon; fichier d'entrée Input1; fichier d'entrée
  // Input2; fichier de sortie et chaine dont l'acceptation est à tester
  bool graphMode = false;     // sortie graphviz ?

  // options acceptées
  const size_t NBOPT = 8;
  string aLN[] = {"accept",
    "determinize",
    "is_terministic",
    "automate2expressionrationnelle",
    "expressionrationnelle2automate",
    "equivalent",
    "no_operation",
    "graph"};
  string aSN[] = {"acc",
    "det",
    "isdet",
    "aut2expr",
    "expr2aut",
    "equ",
    "nop",
    "g"};

  // on essaie de "parser" chaque option de la ligne de commande
  for (int i = 1; i < argc; ++i)
  {
    if (DEBUG)
    {
      cerr << "argv[" << i << "] = '" << argv[i] << "'" << endl;
    }
    str = argv[i];
    pos = -1;
    string* pL = find(aLN, aLN + NBOPT, str.substr(1));
    string* pS = find(aSN, aSN + NBOPT, str.substr(1));

    if (pL != aLN + NBOPT)
    {
      pos = pL - aLN;
    }
    if (pS != aSN + NBOPT)
    {
      pos = pS - aSN;
    }

    if (pos != -1)
    {
      // (pos != -1) <=> on a trouvé une option longue ou courte
      if (DEBUG)
      {
        cerr << "Key found (" << pos << ") : " << str << endl;
      }
      switch (pos)
      {
        case 0: //acc
        case 1: //det
        case 4: //expr2aut
        case 5: //equ
        case 6: //nop
          if (argc < i + 3)
          {
            cerr << "il manque des arguments." << endl;
            return EXIT_FAILURE;
          }
          break;
        case 2: //isdet
        case 3: //aut2expr
          if (argc < i + 2)
          {
            cerr << "il manque des arguments." << endl;
            return EXIT_FAILURE;
          }
          break;
      }
      switch (pos)
      {
        case 0: //acc
          in1 = argv[++i];
          acc = argv[++i];
          nb_ifiles = 1;
          nb_ofiles = 0;
          break;
        case 1: //det
          in1 = argv[++i];
          out = argv[++i];
          nb_ifiles = 1;
          nb_ofiles = 1;
          break;
        case 2: //isdet
          in1 = argv[++i];
          nb_ifiles = 1;
          nb_ofiles = 0;
          break;
        case 3: //aut2expr
          in1 = argv[++i];
          nb_ifiles = 1;
          nb_ofiles = 0;
          break;
        case 4: //expr2aut
          expr = argv[++i];
          out = argv[++i];
          nb_ifiles = 0;
          nb_ofiles = 1;
          break;
        case 5: //equ
          in1 = argv[++i];
          in2 = argv[++i];
          nb_ifiles = 2;
          nb_ofiles = 0;
          break;
        case 6: //nop
          in1 = argv[++i];
          out = argv[++i];
          nb_ifiles = 1;
          nb_ofiles = 1;
          break;
        case 7: //g
          graphMode = true;
          break;
        default:
          return EXIT_FAILURE;
      }
    }
    else
    {
      cerr << "Option inconnue " << str << endl;
      return EXIT_FAILURE;
    }

    if (pos<7)
    {
      if(act > -1)
      {
        cerr << "Plusieurs actions spécifiées" << endl;
        return EXIT_FAILURE;
      }
      else
      {
        act = pos;
      }
    }
  }

  if (act == -1)
  {
    cerr << "Pas d'action spécifiée" << endl;
    return EXIT_FAILURE;
  }

  /* Les options sont OK, on va essayer de lire le(s) automate(s) at1 (et at2)
     et effectuer l'action spécifiée. Atr stockera le résultat*/

  sAutoNDE at1, at2, atr;

  // lecture du des fichiers en entrée
  if ((nb_ifiles == 1 or nb_ifiles == 2) and !FromFile(at1, in1))
  {
    cerr << "Erreur de lecture " << in1 << endl;
    return EXIT_FAILURE;
  }
  if (nb_ifiles ==2 and !FromFile(at2, in2))
  {
    cerr << "Erreur de lecture " << in2 << endl;
    return EXIT_FAILURE;
  }

  switch(act)
  {
    case 0: //acc
      if (Accept(at1, acc))
      {
        cout << "'" << acc << "' est accepté : OUI\n";
      }
      else
      {
        cout << "'" << acc << "' est accepté : NON\n";
      }
      break;
    case 1: //det
      atr = Determinize(at1);
      break;
    case 2: //isdet
      if (EstDeterministe(at1))
      {
        cout << "l'automate fourni en entrée est déterministe : OUI\n";
      }
      else
      {
        cout << "l'automate fourni en entrée est déterministe : NON\n";
      }
      break;
    case 3: //aut2expr
      expr =  Automate2ExpressionRationnelle(at1);
      cout << "Expression rationnelle résultante :" << endl << expr << endl;
      break;
    case 4: //expr2aut
      atr =  ExpressionRationnelle2Automate(expr);
      break;
    case 5: //equ
      if (Equivalent(at1,at2))
      {
        cout << "les deux automates sont équivalents : OUI\n";
      }
      else
      {
        cout << "les deux automates sont équivalents : NON\n";
      }
      break;
    case 6: //nop
      atr = at1;
      break;
    default:
      return EXIT_FAILURE;
  }

  if (nb_ofiles == 1)
  {
    // on affiche le résultat
    // cout << "Automate résultat :\n----------------\n";
    // cout << atr;

    // écriture dans un fichier texte
    ofstream f((out + ".txt").c_str(), ios::trunc);
    if (f.fail())
    {
      return EXIT_FAILURE;
    }
    f << atr;

    // génération d'un fichier graphviz
    if (graphMode)
    {
      ToGraph(atr, out + ".gv");
      system(("dot -Tpng " + out + ".gv -o " + out + ".png").c_str());
    }
  }

  return EXIT_SUCCESS;
}
