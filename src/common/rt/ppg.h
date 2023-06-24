#pragma once

#include <vector>
#include <iostream>
#define STREE_CHILD_NODE 2
#define DTREE_CHILD_NODE 4

#define DTREE_MAX_NODE_BIT 9
#define GET_DTREE_INDEX(x) ((x)>>DTREE_MAX_NODE_BIT)
#define GET_DTREE_ROOT_INDEX(x) static_cast<int>(((x)>>DTREE_MAX_NODE_BIT)<<DTREE_MAX_NODE_BIT)
#define GET_DTREE_ROOT_INDEX_BY_STREE_INDEX(x) static_cast<int>((x)<<DTREE_MAX_NODE_BIT)

class DTree;
//// STree

struct Position {
    float v[3];
};

struct Interval3D {
    float v[3][2];

    friend std::ostream& operator << (std::ostream& out, Interval3D &p);
};

/// <summary>
/// [0, 1]^3
/// </summary>
class STree {
public:
    const static int MAX_NODE;
    static DTree* __root;
    static int __node_index;
    static std::vector<int> __flux;
    static int __trained_spp;

    STree();

    int get_index();

    /// <summary>
    /// only update the flux of the leaf node
    /// </summary>
    int find_index(int depth, int index, Position p);

    /// <summary>
    /// only update the leaf node
    /// </summary>
    void update(int depth, int index, int threshold);
    void print(int index, int depth, Interval3D p = { 0.0,1.0f,0.0,1.0f,0.0,1.0f });
    void initial_split(int index, int depth);

    // depth++: x(0) -> y(1) -> z(2) ->x(3)
    // 0 < 1
    int _child_index[STREE_CHILD_NODE];
    int ___padding[2];

};

//// DTree

struct DInterval {
    float _theta[2], _phi[2];
    friend std::ostream& operator << (std::ostream& out, DInterval& p);
};

class DTree {
public:
    const static int MAX_NODE;
    const static float __rho;
    static std::vector<int> __node_index;
    static DTree* __root;
    static int DTree::get_root_index_by_STree_index(int index);

    // all `index` is absolute index

    /// <summary>
    /// -1 means no space, use `index` to find which DTree it is
    /// </summary>
    int get_index(int index);
    int get_root_index(int index);

    DTree();

    /// <summary>
    /// flux: should be added by this node
    /// </summary>
    void update(const int index, const float flux);
    void fill(const int index, const float theta, const float phi, const float Li, const DInterval angles);
    void initial_split(int index, int depth);

    void copy(int src_index, int dst_index);

    void print(int index, int depth, DInterval degrees = { 0.0,1.0f,0.0,1.0f });


    //  t1 < t2, p1 < p2
    //  0: (t1, p1)
    //  1: (t1, p2)
    //  2: (t2, p1)
    //  3: (t2, p2)
    int _child_index[DTREE_CHILD_NODE];
    float _flux;
    float ___padding[3];
};