// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
//
// When stable, this file should get optimized even in debug
// as pathfinder.cpp previously did.
//
#include "path_finder_llsr.h"
#include "path_finder.h"
#include "map.h"

namespace Glest { namespace Game {

AnnotatedMap* LowLevelSearch::aMap = NULL;
Map* LowLevelSearch::cMap = NULL;
NodePool LowLevelSearch::nodePool;

bool LowLevelSearch::canPathOut ( const Vec2i &pos, const int radius, Field field  )
{
   assert ( radius > 0 && radius <= 5 );
   nodePool.reset ();
   nodePool.addToOpen ( NULL, pos, 0 );
	bool pathFound= false;
   SearchNode *maxNode = NULL;

   while( ! pathFound ) 
   {
      maxNode = nodePool.getBestCandidate ();
      if ( ! maxNode ) break; // failure
		for ( int i = -1; i <= 1 && ! pathFound; ++i )
      {
			for ( int j = -1; j <= 1 && ! pathFound; ++j )
         {
            if ( ! (i||j) ) continue;
            Vec2i sucPos = maxNode->pos + Vec2i(i, j);
            if ( ! cMap->isInside ( sucPos ) 
            ||   ! cMap->getCell( sucPos )->isFree( field == mfAir ? fAir: fSurface ) )
               continue;
            //CanOccupy() will be cheapest, do it first...
            if ( aMap->canOccupy (sucPos, 1, field) && ! nodePool.isListed (sucPos) )
            {
               if ( maxNode->pos.x != sucPos.x && maxNode->pos.y != sucPos.y ) // if diagonal move
               {
                  Vec2i diag1 ( maxNode->pos.x, sucPos.y );
                  Vec2i diag2 ( sucPos.x, maxNode->pos.y );
                  // and either diag cell is not free...
                  if ( ! aMap->canOccupy ( diag1, 1, field ) 
                  ||   ! aMap->canOccupy ( diag2, 1, field )
                  ||   ! cMap->getCell( diag1 )->isFree( field == mfAir ? fAir: fSurface ) 
                  ||   ! cMap->getCell( diag2 )->isFree( field == mfAir ? fAir: fSurface ) )
                     continue; // not allowed
               }
               // Move is legal.
               if ( -(maxNode->heuristic) + 1 >= radius ) 
                  pathFound = true;
               else
                  nodePool.addToOpen ( maxNode, sucPos, maxNode->heuristic - 1.f );
				} // end if
			} // end for
		} // end for
	} // end while
   return pathFound;
}

bool LowLevelSearch::LHF_Search ( SearchParams params, list<Vec2i> &path, bool ucStart )
{
   Vec2i &startPos = params.start;
   Vec2i &finalPos = params.dest;
   int &size = params.size;
   int &team = params.team;
   Field &field = params.field;
   Zone zone = field == mfAir ? fAir : fSurface;
   
   path.clear ();

	//a) push starting pos into openNodes
   nodePool.addToOpen ( NULL, startPos, heuristic (startPos, finalPos) );

   //b) loop
	bool pathFound= true;
	bool nodeLimitReached= false;
	SearchNode *minNode= NULL;

   Vec2i ucPos = ucStart ? startPos : finalPos;
   while( ! nodeLimitReached ) 
   {
      minNode = nodePool.getBestCandidate ();
      if ( ! minNode ) // open was empty?
         return false;

      if ( minNode->pos == finalPos || ! minNode->exploredCell )
			break;

		for(int i=-1; i<=1 && !nodeLimitReached; ++i)
      {
			for(int j=-1; j<=1 && !nodeLimitReached; ++j)
         {
            if ( ! (i||j) ) // i==j==0 == minNode, minor performance hack, 
               continue;   // wouldn't get 'picked up' until nPool.isListed() otherwise...
            Vec2i sucPos = minNode->pos + Vec2i(i, j);
            if ( ! cMap->isInside ( sucPos ) ) 
               continue;
            if ( minNode->pos.dist ( ucPos ) < 5.f && !cMap->getCell( sucPos )->isFree(zone) && sucPos != ucPos )
               continue; // check nearby units
            // CanOccupy () will be cheapest, do it first...
            if ( aMap->canOccupy (sucPos, size, field ) && ! nodePool.isListed ( sucPos ) )
            {
               if ( minNode->pos.x != sucPos.x && minNode->pos.y != sucPos.y ) 
               {  // if diagonal move and either diag cell is not free...
                  Vec2i diag1, diag2;
                  getPassthroughDiagonals ( minNode->pos, sucPos, size, diag1, diag2 );
                  if ( !aMap->canOccupy ( diag1, 1, field ) 
                  ||   !aMap->canOccupy ( diag2, 1, field ) 
                  ||  (minNode->pos.dist(ucPos) < 5.f  && !cMap->getCell (diag1)->isFree (zone) && diag1 != ucPos)
                  ||  (minNode->pos.dist(ucPos) < 5.f  && !cMap->getCell (diag2)->isFree (zone) && diag2 != ucPos) )
                     continue; // not allowed
               }
               // else move is legal.
               bool exp = cMap->getTile (Map::toTileCoords (sucPos))->isExplored (team);
               if ( ! nodePool.addToOpen ( minNode, sucPos, heuristic ( sucPos, finalPos ), exp ) )
                  nodeLimitReached = true;
            }
			} // end for
		} // end for ... inner loop
	} // end while ... outer loop
	SearchNode *lastNode= minNode;
   // if ( nodeLimtReached ) iterate over closed list, testing for a lower h node ...

   //if ( nodeLimitReached ) Logger::getInstance ().add ( "Node Limit Exceeded." );
	// on the way
   // fill in next pointers
	SearchNode *currNode = lastNode;
   int steps = 0;
	while ( currNode->prev )
   {
		currNode->prev->next = currNode;
		currNode = currNode->prev;
      steps++;
	}
   SearchNode *firstNode = currNode;

   //store path
	currNode = firstNode;
   while ( currNode ) 
   {
      path.push_back ( currNode->pos );
      currNode = currNode->next;
   }
   return true;
}

bool LowLevelSearch::LHF_PingPong ( SearchParams params, list<Vec2i> &path )
{
   list<Vec2i> forward, backward, cross;
   if ( ! LHF_Search ( params, forward ) )
		return false;
#ifdef PATHFINDER_DEBUG_TEXTURES
   cMap->ClearPathPos ();
   for ( VLIt it = forward.begin(); it != forward.end(); ++it )
      cMap->SetPathPos ( *it );
   cMap->PathStart = forward.front ();
   cMap->PathDest = forward.back ();
#endif
   nodePool.reset ();// == 800 nodes
   params.dest = params.start;
   params.start = forward.back(); // is not necessarily targetPos, ie. if nodeLimit was hit
   
   if ( LHF_Search ( params, backward, false ) )
   {
#ifdef PATHFINDER_DEBUG_TEXTURES
      for ( VLIt it = backward.begin(); it != backward.end(); ++it )
         cMap->SetPathPos ( *it, true );
#endif
      if ( backward.back() == forward.front() ) // LHF_Search() doesn't guarantee 'symmetrical success'
      {
         getCrossOverPoints ( forward, backward, cross );
         mergePath ( forward, backward, cross, path );
      }
      else
      {  // Shouldn't happen too often now the backward run is gauranteed at least
         // 100 more nodes than the foward run, but is inevitable in some cases
         Logger::getInstance().add ( "MergePath() Failed, backward run failed to reach origin." );
         copyToPath ( forward, path );
      }
   }
   else
      copyToPath ( forward, path );
   return true;
}

void LowLevelSearch::copyToPath ( const list<Vec2i> &pathList, list<Vec2i> &path )
{
   for ( VLConIt it = pathList.begin (); it != pathList.end(); ++it )
      path.push_back ( *it );
}

void LowLevelSearch::getPassthroughDiagonals ( const Vec2i &s, const Vec2i &d, 
                                          const int size, Vec2i &d1, Vec2i &d2 )
{
   assert ( s.x != d.x && s.y != d.y );
   if ( size == 1 )
   {
      d1.x = s.x; d1.y = d.y;
      d2.x = d.x; d2.y = s.y;
      return;
   }
   if ( d.x > s.x )
   {  // travelling east
      if ( d.y > s.y )
      {  // se
         d1.x = d.x + size - 1; d1.y = s.y;
         d2.x = s.x; d2.y = d.y + size - 1;
      }
      else
      {  // ne
         d1.x = s.x; d1.y = d.y;
         d2.x = d.x + size - 1; d2.y = s.y - size + 1;
      }
   }
   else
   {  // travelling west
      if ( d.y > s.y )
      {  // sw
         d1.x = d.x; d1.y = s.y;
         d2.x = s.x + size - 1; d2.y = d.y + size - 1;
      }
      else
      {  // nw
         d1.x = d.x; d1.y = s.y - size + 1;
         d2.x = s.x + size - 1; d2.y = d.y;
      }
   }
}

void LowLevelSearch::getCrossOverPoints ( const list<Vec2i> &forward, const list<Vec2i> &backward, list<Vec2i> &result )
{
   result.clear ();
   nodePool.reset ();
   for ( VLConRevIt it1 = backward.rbegin(); it1 != backward.rend(); ++it1 )
      nodePool.listPos ( *it1 );
   for ( VLConIt it2 = forward.begin(); it2 != forward.end(); ++it2 )
      if ( nodePool.isListed ( *it2 ) )
         result.push_back ( *it2 );
}

bool LowLevelSearch::mergePath ( const list<Vec2i> &fwd, const list<Vec2i> &bwd, list<Vec2i> &co, list<Vec2i> &path )
{
   assert ( co.size () <= fwd.size () );

   if ( !path.empty () ) path.clear ();

   if ( fwd.size () == co.size () ) 
   {  // paths never diverge
      copyToPath ( fwd, path );
      return true;
   }
   VLConIt fIt = fwd.begin ();
   VLConRevIt bIt = bwd.rbegin ();
   VLIt coIt = co.begin ();

   //the 'first' and 'last' nodes on fwd and bwd must be the first and last on co...
   assert ( *coIt == *fIt && *coIt == *bIt );
   assert ( co.back() == fwd.back() && co.back() == bwd.front() );
   if ( ! ( *coIt == *fIt && *coIt == *bIt ) )
   {
      //throw new runtime_error ( "LowLevelSearch::MergePath() was passed dodgey data..." );
      Logger::getInstance().add ( "LowLevelSearch::MergePath() was passed dodgey data..." );
      copyToPath ( fwd, path );
      return false;
   }
   // push start pos
   path.push_back ( *coIt );

   ++fIt; ++bIt;
   // Should probably just iterate over co aswell...
   coIt = co.erase ( coIt );

   while ( coIt != co.end () )
   {
      // coIt now points to a pos that is common to both paths, but isn't the start... 
      // skip any more duplicates, putting them onto the path
      while ( *coIt == *fIt )
      {
         path.push_back ( *coIt );
         if ( *fIt != *bIt )
         {
            //throw new runtime_error ( "LowLevelSearch::MergePath() was passed a dodgey crossover list..." );
            Logger::getInstance().add ( "LowLevelSearch::MergePath() was passed a dodgey crossover list..." );
            if ( !path.empty () ) path.clear ();
            copyToPath ( fwd, path );
            return false;
         }
         coIt = co.erase ( coIt );
         if ( coIt == co.end() ) // that's it folks!
            return true;
         ++fIt; ++bIt;
      }
      // coIt now points to the next common pos, 
      // fIt and bIt point to the positions where the path's have just diverged
      int fGap = 0, bGap = 0;

      VLConIt fStart = fIt; // save our spot
      VLConRevIt bStart = bIt; // ditto
      while ( *fIt != *coIt ) { fIt ++; fGap ++; }
      while ( *bIt != *coIt ) { bIt ++; bGap ++; }
      if ( bGap < fGap )
      {  // copy section from bwd
         while ( *bStart != *coIt )
         {
            path.push_back ( *bStart );
            bStart ++;
         }
      }
      else
      {  // copy section from fwd
         while ( *fStart != *coIt )
         {
            path.push_back ( *fStart );
            fStart ++;
         }
      }
      // now *fIt == *bIt == *coIt... skip duplicates, etc etc...
   } // end while ( coIt != co.end () )
   Logger::getInstance().add ( "in LowLevelSearch::MergePath() your 'unreachable' code was reached... ?!?" );
   return true; // keep the compiler happy...
}

SearchParams::SearchParams ( Unit *u ) 
{
   start = u->getPos(); 
   field = u->getCurrField ();
   size = u->getSize (); 
   team = u->getTeam ();
}

PosTree::PosTree ()
{
   numPos = 0;
   posStock = new PosTreeNode[PathFinder::pathFindNodesMax];
   sentinel.colour = PosTreeNode::Black;
   sentinel.left = sentinel.right = sentinel.parent = NULL;
   sentinel.pos = Vec2i(-1,-1);
   posRoot = NULL;
}

PosTree::~PosTree ()
{
   delete posStock;
}

//
// A regular tree insert, the new node is coloured explicitly
// if it's the root it's coloured black and we're done, otherwise the new node is coloured
// Red, inserted then passed to rebalance() to fix any Red-Black property violations
//
void PosTree::add ( const Vec2i &pos )
{
   posStock[numPos].pos = pos;
   posStock[numPos].left = posStock[numPos].right = &sentinel;
   
   if ( ! numPos )
   {
      posRoot = &posStock[0];
      posStock[0].parent = &sentinel;
      posStock[0].colour = PosTreeNode::Black;
   }
   else
   {
      PosTreeNode *parent = posRoot;
      posStock[numPos].colour = PosTreeNode::Red;
      while ( true )
      {
         if ( pos.x < parent->pos.x 
         ||   ( pos.x == parent->pos.x && pos.y < parent->pos.y ) )
         {  // go left
            if ( parent->left == &sentinel )
            {  // insert and break
               parent->left = &posStock[numPos];
               posStock[numPos].parent = parent;
               rebalance ( &posStock[numPos] );
               break;
            }
            else parent = parent->left;
         }
         else if ( pos.x > parent->pos.x 
         ||   ( pos.x == parent->pos.x && pos.y > parent->pos.y ) )
         {  // go right
            if ( parent->right == &sentinel )
            {  // insert and break
               parent->right = &posStock[numPos];
               posStock[numPos].parent = parent;
               rebalance ( &posStock[numPos] );
               break;
            }
            else parent = parent->right;
         }
         else // pos == parent->pos ... Error
            throw new runtime_error ( "Duplicate position added to PathFinder::NodePool." );
      }
   }
   numPos++;
}

#define DAD(x) (x->parent)
#define GRANDPA(x) (x->parent->parent)
#define UNCLE(x) (x->parent->parent?x->parent->parent->left==x->parent\
                  ?x->parent->parent->right:x->parent->parent->left:NULL)
//
// Restore Red-Black properties on PosTree, with 'node' having just been inserted
//
void PosTree::rebalance ( PosTreeNode *node )
{
   while ( DAD(node)->colour == PosTreeNode::Red )
   {
      if ( UNCLE(node)->colour == PosTreeNode::Red )
      {
         // case 'Red Uncle', colour flip dad, uncle, and grandpa then restart with grandpa
         DAD(node)->colour = UNCLE(node)->colour = PosTreeNode::Black;
         GRANDPA(node)->colour = PosTreeNode::Red;
         assert ( GRANDPA(node) != &sentinel );
         node = GRANDPA(node);
      }
      else // UNCLE(node) is black
      {
         if ( DAD(node) == GRANDPA(node)->left )
         {
            // dad red left child, uncle black
            if ( node == DAD(node)->right )
            {
               node = DAD(node);
               rotateLeft ( node );
            }
            DAD(node)->colour = PosTreeNode::Black;
            GRANDPA(node)->colour = PosTreeNode::Red;
            assert ( GRANDPA(node) != &sentinel );
            rotateRight ( GRANDPA(node) ); 
         }
         else // Dad is right child of grandpa
         {
            // dad red right child, uncle black
            PosTreeNode *prevDad = DAD(node), *prevGrandpa = GRANDPA(node);
            if ( node == DAD(node)->left )
            {
               node = DAD(node);
               rotateRight ( node );
            }
            DAD(node)->colour = PosTreeNode::Black;
            GRANDPA(node)->colour = PosTreeNode::Red;
            assert ( GRANDPA(node) != &sentinel );
            rotateLeft ( GRANDPA(node) );
         } // end if..else, dad left or right child of grandpa
      } // end if..else, uncle red or black
   } // end while, dad red
   posRoot->colour = PosTreeNode::Black; // node is new root
}

#if defined(DEBUG) || defined(_DEBUG)
bool PosTree::assertValidity ()
{
   if ( !posRoot ) return true;
   Vec2i low = Vec2i ( -1,-1 );
   // Inorder traversal, ref [Algorithm T, Knuth Vol 1 pg 317]
   // T1 [Initialize]
   list<PosTreeNode*> stack; // stupid VC++ doesn't have stack...
   PosTreeNode *ptr = posRoot;
   while ( true )
   {
      // T2 [if P == NULL goto T4]
      if ( ptr != &sentinel ) 
      {
         // T3 [stack.push(P), P = P->left, goto T2]
         stack.push_back ( ptr );
         ptr = ptr->left;
         continue;
      }
      // T4 [if stack.empty() terminate, else P = stack.pop()]
      if ( stack.empty () ) 
         break;
      ptr = stack.back ();
      stack.pop_back ();
      //
      // T5 [Visit P, P = P->right, goto T2]
      if ( ptr->colour == PosTreeNode::Red 
      &&   ( ptr->left->colour == PosTreeNode::Red || ptr->right->colour == PosTreeNode::Red ) )
      {
         Logger::getInstance ().add ( "Red-Black Tree Invalid, Red-Black Property violated." );
         dump ();
         return false;
      }
      if ( ptr->pos.x < low.x || ( ptr->pos.x == low.x && ptr->pos.y < low.y ) )  
      {
         Logger::getInstance ().add ( "Search Tree Invalid! Elements out of order." );
         dump ();
         return false;
      }
      low = ptr->pos;
      ptr = ptr->right;
   }
   return true;
}
void PosTree::dump ()
{
   static char buf[1024*4];
   char *ptr = buf;
   if ( !posRoot )
   {
      Logger::getInstance ().add ( "Tree Empty." );
      return;
   }
   list<PosTreeNode*> *thisLevel = new list<PosTreeNode*>();
   list<PosTreeNode*> *nextLevel = NULL;
   thisLevel->push_back ( posRoot );
   while ( ! thisLevel->empty () )
   {
      ptr += sprintf ( ptr, "\n" );
      nextLevel = new list<PosTreeNode*>();
      for ( list<PosTreeNode*>::iterator it = thisLevel->begin(); it != thisLevel->end(); ++it )
      {
         ptr += sprintf ( ptr, "[%d,%d|", (*it)->pos.x, (*it)->pos.y );
         if ( (*it)->left )
         {
            ptr += sprintf ( ptr, "L:%d,%d|", (*it)->left->pos.x, (*it)->left->pos.y );
            nextLevel->push_back ( (*it)->left );
         }
         else ptr += sprintf ( ptr, "L:NIL|" );
         if ( (*it)->right )
         {
            ptr += sprintf ( ptr, "R:%d,%d|", (*it)->right->pos.x, (*it)->right->pos.y );
            nextLevel->push_back ( (*it)->right );
         }
         else ptr += sprintf ( ptr, "R:NIL|" );
         ptr += sprintf ( ptr, "%s] ", (*it)->colour ? "Black" : "Red" );

      }
      delete thisLevel;
      thisLevel = nextLevel;
   }
   delete thisLevel;
   Logger::getInstance ().add ( buf );
}
#endif

// Tree rotations, ref [Cormen, 13.2]
#define ROTATE_ERR_MSG "PathFinder::NodePool::rotateLeft() was called on a node with no right child."
void PosTree::rotateLeft ( PosTreeNode *root )
{
   PosTreeNode *pivot = root->right; // 1
   assert ( pivot != &sentinel );
   //if ( pivot == &sentinel ) 
   //   throw new runtime_error ( ROTATE_ERR_MSG );
   root->right = pivot->left;  // 2
   root->right->parent = root; // 3
   pivot->parent = root->parent; // 4
   root->parent = pivot; // 11
   pivot->left = root; // 10
   if ( pivot->parent != &sentinel ) // 5
   {
      if ( pivot->parent->left == root ) // 7
         pivot->parent->left = pivot; // 8
      else
         pivot->parent->right = pivot; // 9
   }
   else posRoot = pivot; // 6
}
#undef ROTATE_ERR_MSG
#define ROTATE_ERR_MSG "PathFinder::NodePool::rotateRight() was called on a node with no left child."
void PosTree::rotateRight ( PosTreeNode *root )
{
   PosTreeNode *pivot = root->left;
   assert ( pivot != &sentinel );
   //if ( pivot == &sentinel ) 
   //   throw new runtime_error ( ROTATE_ERR_MSG );
   root->left = pivot->right;
   root->left->parent = root;
   pivot->parent = root->parent;
   root->parent = pivot;
   pivot->right = root;
   if ( pivot->parent != &sentinel)
   {
      if ( pivot->parent->left == root )
         pivot->parent->left = pivot;
      else
         pivot->parent->right = pivot;
   }
   else posRoot = pivot;
}
#undef ROTATE_ERR_MSG

// is pos already listed?
bool PosTree::isIn ( const Vec2i &pos ) const
{
   PosTreeNode *ptr = posRoot;
   while ( ptr != &sentinel)
   {
      if ( pos.x < ptr->pos.x )
         ptr = ptr->left;
      else if ( pos.x > ptr->pos.x )
         ptr = ptr->right;
      else //  pos.x == ptr->pos.x 
      {
         if ( pos.y < ptr->pos.y )
            ptr = ptr->left;
         else if ( pos.y > ptr->pos.y ) 
            ptr = ptr->right;
         else // pos == ptr->pos
            return true;
      }
   }

   return false;
}


NodePool::NodePool ()
{
   maxNodes = PathFinder::pathFindNodesMax;
   stock = new SearchNode[maxNodes];
   lists = new SearchNode*[maxNodes];
   reset ();
}

NodePool::~NodePool () 
{
   delete stock;
   delete lists;
}
void NodePool::init ( Map *map )
{
#ifdef NODEPOOL_USE_MARKER_ARRAY
   markerArray.init ( map->getW(), map->getH() );
#endif
}
// reset the node pool
void NodePool::reset ()
{
   numOpen = numClosed = numTotal = numPos = 0;
   tmpMaxNodes = maxNodes;
#if defined ( NODEPOOL_USE_MARKER_ARRAY )
   markerArray.newSearch ();
#elif defined ( NODEPOOL_USE_REDBLACK_TREE )
   posTree.clear ();
#endif
}

void NodePool::setMaxNodes ( const int max )
{
   assert ( max >= 50 && max <= maxNodes ); // reasonable number ?
   assert ( !numTotal ); // can't do this after we've started using it.
   tmpMaxNodes = max;
}

bool NodePool::addToOpen ( SearchNode* prev, const Vec2i &pos, float h, bool exp )
{
   if ( numTotal == tmpMaxNodes ) 
      return false;
   stock[numTotal].next = NULL;
   stock[numTotal].prev = prev;
   stock[numTotal].pos = pos;
   stock[numTotal].heuristic = h;
   stock[numTotal].exploredCell = exp;
   const int top = tmpMaxNodes - 1;
   if ( !numOpen ) lists[top] = &stock[numTotal];
   else
   {  // find insert index
      // due to the nature of the modified A*, new nodes are likely to have lower heuristics
      // than the majority already in open, so we start checking from the low end.
      const int openStart = tmpMaxNodes - numOpen - 1;
      int offset = openStart;

      while ( offset < top && lists[offset+1]->heuristic < stock[numTotal].heuristic ) 
         offset ++;

      if ( offset > openStart ) // shift lower nodes down...
      {
         int moveNdx = openStart;
         while ( moveNdx <= offset )
         {
            lists[moveNdx-1] = lists[moveNdx];
            moveNdx ++;
         }
      }
      // insert newbie in sorted pos.
      lists[offset] = &stock[numTotal];
   }
#if defined ( NODEPOOL_USE_REDBLACK_TREE )
      posTree.add ( pos );
#elif defined ( NODEPOOL_USE_MARKER_ARRAY )
      markerArray.setMark ( pos );
#endif
   numTotal ++;
   numOpen ++;
   return true;
}

// Moves the lowest heuristic node from open to closed and returns a 
// pointer to it, or NULL if there are no open nodes.
SearchNode* NodePool::getBestCandidate ()
{
   if ( !numOpen ) return NULL;
   lists[numClosed] = lists[tmpMaxNodes - numOpen];
   numOpen --;
   numClosed ++;
   return lists[numClosed-1];
}

}}
