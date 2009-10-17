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
//
// File: node_pool.h
//

#ifndef _GLEST_GAME_PATHFINDER_NODE_POOL_H_
#define _GLEST_GAME_PATHFINDER_NODE_POOL_H_

#include "vec.h"
#include "timer.h"
#include "unit_stats_base.h"

#include <set>
#include <limits>

using Shared::Platform::Chrono;
using std::numeric_limits;
using std::set;

namespace Glest { namespace Game { 

class Unit;
class Map;

namespace Search {

#pragma pack(push, 4)
struct PosOff {				/**< A bit packed position (Vec2i) and offset (direction) pair		 */
	PosOff() : x(0), y(0), ox(0), oy(0) {}				 /**< Construct a PosOff [0,0]			*/
	PosOff(Vec2i pos) : ox(0),oy(0) { *this = pos; }	/**< Construct a PosOff [pos.x, pos.y] */
	PosOff(int x, int y) : x(x) ,y(y) ,ox(0), oy(0) {} /**< Construct a PosOff [x,y]		  */
	PosOff& operator=(Vec2i pos) {					  /**< Assign from Vec2i				 */
		assert(pos.x <= 8191 && pos.y <= 8191);
		x = pos.x; y = pos.y; return *this;
	}
	bool operator==(PosOff &that) { return x == that.x && y == that.y; } /**< compare position components only */
	Vec2i getPos()		{ return Vec2i(x, y); }				/**< this packed pos as Vec2i  */
	Vec2i getPrev()		{ return Vec2i(x + ox, y + oy); }  /**< return pos + offset		  */
	Vec2i getOffset()	{ return Vec2i(ox, oy); }		  /**< return offset			 */
	bool hasOffset()	{ return ox || oy; }			 /**< has an offset				*/
	bool valid() { return x >= 0 && y >= 0; }			/**< is this position valid	   */

	int32  x : 14; /**< x coordinate  */
	int32  y : 14; /**< y coordinate */
	int32 ox :  2; /**< x offset	*/
	int32 oy :  2; /**< y offset   */
};
#pragma pack(2)
struct NodeID {		  // max 65536 'addressable' nodes
	uint16 pool	:  4; // max 16 pools
	uint16 ndx	: 12; // max pool size 4096
};
#pragma pack(pop)

// =====================================================
// struct AStarNode
// =====================================================
struct AStarNode {					/**< A node structure for A* with NodeStore							*/
	PosOff posOff;				   /**< position of this node, and direction of best path to it		   */
	float heuristic;			  /**< estimate of distance to goal									  */
	float distToHere;			 /**< cost from origin to this node									 */
	NodeID nextOpen;			/**< index of next open node, valid of this node is on the openList */
	float est()	const { return distToHere + heuristic;}	   /**< estimate, costToHere + heuristic   */
	Vec2i pos()		  { return posOff.getPos();		  }	  /**< position of this node			  */
	Vec2i prev()	  { return posOff.getPrev();	  }  /**< best path to this node is from	 */
	bool hasPrev()	  { return posOff.hasOffset();	  } /**< has valid previous 'pointer'		*/
}; // == 112 bits ( == 128 aligned )

// =====================================================
// class AStarComp
// =====================================================
class AStarComp { /**< Comparison function for the open heap @todo deprecate, replace heap */
public:
	/** Comparison function
	  * @param one AStarNode for comparison
	  * @param two AStarNode for comparison
	  * @return  true if two is 'better' than one
	  */
	bool operator()( const AStarNode * const one, const AStarNode * const two ) const {
		const float diff = ( one->distToHere + one->heuristic ) - ( two->distToHere + two->heuristic );
		if ( diff < 0 ) return false;
		else if ( diff > 0 ) return true;
		// tie, prefer closer to goal...
		if ( one->heuristic < two->heuristic ) return false;
		if ( one->heuristic > two->heuristic ) return true;
		// still tied... prefer nodes 'in line' with goal ???
		// just distinguish them somehow...
		return one < two;
	}
};

#if 1
/** A 'split' list, a sorted head (limited size) and unsorted bucket */
class OpenList {
private:
	static const int maxHeadSize = 16;

	/** A sorted doubly linked list with limited storage */
	class Head {
		struct Node {
			AStarNode *data;
			Node *next, *prev;
		};
		Node *start, *end;//, *middle;
		float maxEstimate;
		Node *block;
		vector<Node*> freeNodes;
		int count;

		void insertBefore(Node *insert, Node *ref) {
			if ( ref->prev ) {
				insert->prev = ref->prev;
				ref->prev->next = insert;
			} else {
				start = insert;
				insert->prev = NULL;
			}
			insert->next = ref;
			ref->prev = insert;
		}

		void insertAtEnd(Node *insert) {
			insert->next = NULL;
			insert->prev = end;
			end = insert;
			if ( insert->prev ) {
				insert->prev->next = insert;
			} else {
				start = insert;
			}
		}

		Node* unlink(Node *node) {
			if ( node->prev) {
				node->prev->next = node->next;
			} else {
				start = node->next;
			}
			if ( node->next ) {
				node->next->prev = node->prev;
			} else {
				end = node->prev;
			}
			return node;
		}

		bool assertList() {
			set<Node*> seen;
			Node *ptr = start;
			while ( ptr ) {
				if ( seen.find(ptr) != seen.end() ) {
					return false; // cycle
				}
				seen.insert(ptr);
				if ( ptr->next ) {
					if ( ptr->data->est() > ptr->next->data->est() ) {
						return false; // not in order
					}
					if ( ptr->next->prev != ptr ) {
						return false; // inconsistant next/prev links
					}
				}
				ptr = ptr->next;
			}
			if ( count != seen.size() ) {
				return false; // count inconsistant
			}
			return true;
		}

	public:
		Head() : start(NULL), end(NULL), maxEstimate(0.f), count(0) {
			block = new Node[maxHeadSize];
			for ( int i=0; i < maxHeadSize; ++i ) {
				freeNodes.push_back(&block[i]);
			}
		}
		~Head() { delete [] block; }

		AStarNode* getBest();
		AStarNode* add(AStarNode *node);
		void addToEnd(AStarNode *node);
		bool empty()	{ return !count; }
		int  size()		{ return count; }
		float  maxEst()	{ return maxEstimate; }
	};
	/** a unsorted collection of Nodes and some information about them */
	class Bucket : public vector<AStarNode*> {
		float minEstimate,	 /**< the lowest estimate of any node in the bucket  */
			  maxEstimate;	/**< the highest estimate of any node in the bucket */
	public:
		Bucket() : minEstimate(numeric_limits<float>::infinity()), maxEstimate(0.f) { }
		void add(AStarNode *n) {
			if ( n->est() < minEstimate ) minEstimate = n->est();
			if ( n->est() > maxEstimate ) maxEstimate = n->est();
			push_back(n);
		}
		void  sort()			{ 
			std::sort(begin(), end(), AStarComp()); 
			// store some info?
			// sort into multiple buckets?
		}
		float minEst() const	{ return minEstimate; }
		float maxEst() const	{ return maxEstimate; }
		void  setMinEst(float val) { minEstimate = val; }
	};

private:
	Head head;
	Bucket bucket;

	int totalSize() { return head.size() + bucket.size(); }

public:
	void push(AStarNode *node) {
		if ( totalSize() < maxHeadSize && bucket.empty() ) {
			AStarNode *displaced = head.add(node);
			assert(!displaced);			
		} else {
			// if ( node->est() < maxInHead ) sort into head else throw in bucket
			if ( node->est() < head.maxEst() ) {
				AStarNode *displaced = head.add(node);
				if ( displaced ) {
					bucket.add(displaced);
				}
			} else {
				bucket.add(node);
			}
		}
	}

	/** @todo ADD CODE */
	void adjust(AStarNode *node, float costToHere) {
		if ( node->est() <= head.maxEst() ) {
			node->distToHere = costToHere;
			// probably in head... not definately
			// if in head, adjust and return
			// else fall through
		}
		node->distToHere = costToHere;
		if ( node->est() < head.maxEst() ) {

		}
	}

	AStarNode* pop() {
		if ( head.empty() ) {
			if ( bucket.empty() ) {
				return NULL;
			}
			fill_head();
		}
		return head.getBest();
	}

private:
	/** refill the head list, <b>pre-condition:</b> head is empty, bucket is not */
	void fill_head() {
		assert(head.empty() && !bucket.empty());
		bucket.sort();
		int num = maxHeadSize;
		if ( bucket.size() < num ) num = bucket.size();
		for ( ; num; --num ) {
			AStarNode *node = bucket.back();
			bucket.pop_back();
			head.addToEnd(node);
		}
		// maintain bucket.minEstimate
		bucket.setMinEst(bucket.back()->est());
	}
};
#endif

// ========================================================
// class NodePool
// ========================================================
/** An array of AStarNode.  
  * A NodePool is attached to a NodeStore and provide the new nodes
  * for a search. */
class NodePool {
private:
	AStarNode *stock; /**< The block of nodes */
	int counter;	 /**< current counter    */
public:
	NodePool() : counter(0) { stock = new AStarNode[size]; } /**< Construct NodePool */
	~NodePool() { delete [] stock; } /**< Delete NodePool							*/
	static const int size = 512;	/**< total number of AStarNodes in each pool   */
	AStarNode*	newNode()	{ return ( counter < size ? &stock[counter++] : NULL ); } 
	void reset() { counter = 0; }
};
// ========================================================
// class NodeStore
// ========================================================
class NodeStore {	/**< A NodeStorage class (template interface) for A* */
private:
	// =====================================================
	// struct DoubleMarkerArray
	// =====================================================
	/** A Marker Array supporting two mark types, open and closed. @todo interleave with pointer array */
	struct DoubleMarkerArray {
	private:
		int stride;				/**< stride of array */
		unsigned int counter;  /**< the counter		*/
		unsigned int *marker; /**< the array	   */
	public:
		DoubleMarkerArray(int w, int h)	: stride(w) { 
			marker = new unsigned int[w * h]; 
			memset(marker, 0, w * h * sizeof(unsigned int)); 
		}
		~DoubleMarkerArray() { delete [] marker; }
		inline void newSearch() { counter += 2; }
		inline void setOpen(const Vec2i &pos)	{ marker[pos.y * stride + pos.x] = counter; }
		inline void setClosed(const Vec2i &pos)	{ marker[pos.y * stride + pos.x] = counter + 1; }
		inline bool isOpen(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] == counter; }
		inline bool isClosed(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] == counter + 1;  }
		inline bool isListed(const Vec2i &pos)	{ return marker[pos.y * stride + pos.x] >= counter; } /**< @deprecated not needed? */		
		inline void setNeither(const Vec2i &pos){ marker[pos.y * stride + pos.x] = 0; } /**< @deprecated not needed? */
	};

	// =====================================================
	// struct PointerArray
	// =====================================================
	/** An array of pointers
	  * <p>Must be used in conjunction with marker array, constantly contains junk 
	  * values, use only if mark > counter</p> */
	struct PointerArray {
	private:
		int stride;			 /**< stride of array */
		AStarNode **pArray;	/**< the array		 */
	public:
		PointerArray(int w, int h) : stride(w) { /**< Construct pointer array */
			pArray = new AStarNode*[w * h]; 
			memset(pArray, NULL, w * h * sizeof(AStarNode*)); 
		}
		~PointerArray() { delete [] pArray; } /**< delete pointer array */
		inline void set(const Vec2i &pos, AStarNode *ptr) { pArray[pos.y * stride + pos.x] = ptr; }	 /**< set the pointer for pos to ptr */
		inline AStarNode* get(const Vec2i &pos) { return pArray[pos.y * stride + pos.x]; }			/**< get the pointer for pos		*/
	};

private:
	AStarNode *leastH;	/**< The 'best' node seen so far this search	*/
	NodePool *pool;	   /**< the current NodePool to get new nodes from */
	int numNodes;	  /**< number of nodes used so far this search	  */
	int tmpMaxNodes; /**< a temporary maximum number of nodes to use */
	
	DoubleMarkerArray markerArray;	/**< An array the size of the map, indicating node status (unvisited, open, closed)		  */
	PointerArray pointerArray;	   /**< An array the size of the map, containing pointers to Nodes, valid if position marked */
	vector<AStarNode*> openHeap;  /**< the open list, binary heap, maintained with std algorithms							*/
	OpenList openList;			 /**< the open list,  'split' list, sorted head and unsorted bucket						   */

public:
	NodeStore();
	~NodeStore();

	void attachNodePool(NodePool *nPool) { pool = nPool; }

	// NodeStorage template interface
	//
	// same as old...
	//void reset();
	//bool isOpen ( const Vec2i &pos );
	//bool isClosed ( const Vec2i &pos );
	bool setOpen(const Vec2i &pos, const Vec2i &prev, float h, float d);
	void updateOpen(const Vec2i &pos, const Vec2i &prev, const float cost);
	/** get the best candidate from the open list, and close it.
	  * @return the lowest estimate node from the open list, or -1,-1 if open list empty */
	Vec2i getBestCandidate() {
		AStarNode *ptr = getBestCandidateNode();
		return ptr ? ptr->pos() : Vec2i(-1); 
	}
	/** get the best heuristic node seen this search */
	Vec2i getBestSeen()						{ return leastH->pos(); }
	/** get the heuristic of the node at pos [known to be visited] */
	float getHeuristicAt(const Vec2i &pos)	{ return pointerArray.get(pos)->heuristic;	}
	/** get the cost to the node at pos [known to be visited] */
	float getCostTo(const Vec2i &pos)		{ return pointerArray.get(pos)->distToHere;	}
	/** get the estimate for the node at pos [known to be visited] */
	float getEstimateFor(const Vec2i &pos)	{ return pointerArray.get(pos)->est();		}
	/** get the best path to the node at pos [known to be visited] */
	Vec2i getBestTo(const Vec2i &pos)		{ 
		AStarNode *ptr = pointerArray.get(pos);
		assert (ptr);
		return ptr->hasPrev() ? ptr->prev() : Vec2i(-1);
	}

	// old algorithm interface...
	void setMaxNodes(const int max);
	void reset();
	/** @deprecated ? do not use ? */
	bool limitReached() { return numNodes == tmpMaxNodes; }
	bool addToOpen(AStarNode *prev, const Vec2i &pos, float h, float d);
	void addOpenNode(AStarNode *node);
	void updateOpenNode(const Vec2i &pos, AStarNode *neighbour, float cost);
	AStarNode* getBestCandidateNode();
	bool isOpen(const Vec2i &pos)	{ return markerArray.isOpen(pos);	}	/**< test if a position is open */
	bool isClosed(const Vec2i &pos) { return markerArray.isClosed(pos); }	/**< test if a position is closed */
	bool isListed(const Vec2i &pos) { return markerArray.isListed(pos); }	/**< @deprecated needed for canPathOut() */
	AStarNode* getBestHNode() { return leastH; }	/** @deprecated use getBestSeen() */

#if DEBUG_SEARCH_TEXTURES
	// interface to support debugging textures
	list<Vec2i>* getOpenNodes();
	list<Vec2i>* getClosedNodes();
	list<Vec2i> listedNodes;
#endif
};

}}}

#endif