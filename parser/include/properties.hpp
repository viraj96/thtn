#ifndef __PROPERTIES

#define __PROPERTIES

#include <vector>

#include "parsetree.hpp"

void printProperties();
bool isRecursiveParentSort(string curr, string target);
vector<string> liftedPropertyTopSort(parsed_task_network *tn);
bool recursionFindingDFS(string cur, map<string, int> &colour);

bool isTopSortTotalOrder(vector<string> &liftedTopSort,
                         parsed_task_network *tn);

void liftedPropertyTopSortDFS(string cur, map<string, vector<string> > &adj,
                              map<string, int> &colour,
                              vector<string> &liftedTopSort);

#endif
