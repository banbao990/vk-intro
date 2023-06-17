#include "ppg.h"

#include <iostream>
#include <cassert>

const int STree::MAX_NODE = 10000;
int STree::__node_index = 0;

#define DTREE_MAX_NODE_BIT 9
#define GET_DTREE_INDEX(x) ((x)>>DTREE_MAX_NODE_BIT)
#define GET_DTREE_ROOT_INDEX(x) static_cast<int>(((x)>>DTREE_MAX_NODE_BIT)<<DTREE_MAX_NODE_BIT)
#define GET_DTREE_ROOT_INDEX_BY_STREE_INDEX(x) static_cast<int>((x)<<DTREE_MAX_NODE_BIT)

std::vector<int> STree::__flux(STree::MAX_NODE, 0);
DTree* STree::__root = nullptr;

const int DTree::MAX_NODE = (1 << DTREE_MAX_NODE_BIT);
const float DTree::__rho = 0.01f;
std::vector<int> DTree::__node_idx(STree::MAX_NODE, 0);
DTree* DTree::__root = nullptr;


std::ostream& operator << (std::ostream& out, Interval3D& p) {
    return out << "["
        << "(" << p.v[0][0] << ", " << p.v[0][1] << "), "
        << "(" << p.v[1][0] << ", " << p.v[1][1] << "), "
        << "(" << p.v[2][0] << ", " << p.v[2][1] << ")"
        << "]";
}

//// STree
int STree::get_index() {
    if (__node_index + STREE_CHILD_NODE >= MAX_NODE) {
        return -1;
    }
    int ret = __node_index + 1;
    __node_index += 2;
    return ret;
}

STree::STree() {
    for (int i = 0; i < STREE_CHILD_NODE; ++i) {
        _child_index[i] = -1;
    }
}

void STree::update(int depth, int index, int threshold) {
    // no child
    if (_child_index[0] == -1) {
        int& flux = __flux[index];
        if (flux < threshold) {
            return;
        }
        // construct child node, abnd fall through(update child node)
        int c_index = get_index();
        // no space left
        if (c_index == -1) {
            return;
        }

        int sub_flux = flux / STREE_CHILD_NODE;
        for (int i = 0; i < STREE_CHILD_NODE; ++i) {
            _child_index[i] = c_index;
            __flux[c_index] = sub_flux;
            assert(__root != nullptr);
            __root->copy(index, c_index);
            ++c_index;
        }
    }

    // have child
    for (int i = 0; i < STREE_CHILD_NODE; ++i) {
        int c_index = _child_index[i];
        (this + c_index - index)->update(depth + 1, c_index, threshold);
    }
    return;
}

int STree::find_index(int depth, int index, Position p) {
    // no child, return itself
    if (_child_index[0] == -1) {
        ++__flux[index];
        return index;
    }

    // have child
    int sub_time = (depth / 3);
    int p_index = depth % 3;

    int c_idx_idx = 1;
    if (p.v[p_index] < 1.0f / (2 << sub_time)) {
        c_idx_idx = 0;
    }

    int c_index = _child_index[c_idx_idx];
    return (this + c_index - index)->find_index(depth + 1, c_index, p);
}


void STree::print(int index, int depth, Interval3D p) {
    int p_index = depth % 3;
    std::cout << std::string(static_cast<int>(depth << 1), ' ') << index
        << ": flux = " << __flux[index]
        << ", " << p << std::endl;
    float p_min = p.v[p_index][0];
    float p_max = p.v[p_index][1];
    float p_mid = (p_min + p_max) / 2;

    if (_child_index[0] != -1) {
        for (int i = 0; i < STREE_CHILD_NODE; ++i) {
            p.v[p_index][0] = (i == 0) ? p_min : p_mid;
            p.v[p_index][1] = (i == 0) ? p_mid : p_max;
            (this + _child_index[i] - index)->print(_child_index[i], depth + 1, p);
        }
    }
}

void STree::initial_split(int index, int depth) {
    if (depth == 0) { return; }
    int idx = get_index();
    // should have space left
    assert(idx != -1);

    for (int i = 0; i < STREE_CHILD_NODE; ++i) {
        _child_index[i] = idx;
        (this + idx - index)->initial_split(idx, depth - 1);
        ++idx;
    }
}

//// DTree

std::ostream& operator << (std::ostream& out, DInterval& p) {
    return out << "["
        << "(" << p._theta[0] << ", " << p._theta[1] << "), "
        << "(" << p._phi[0] << ", " << p._phi[1] << ")"
        << "]";
}

DTree::DTree() {
    for (int i = 0; i < DTREE_CHILD_NODE; ++i) {
        _child_index[i] = -1;
    }
    _flux = 0;
}

void DTree::fill(int index, float theta, float phi, float Li, DInterval angles) {
    _flux += Li;
    // no child
    if (_child_index[0] == -1) { return; }

    float t1 = angles._theta[0];
    float t2 = angles._theta[1];
    float p1 = angles._phi[0];
    float p2 = angles._phi[1];
    float tm = (t1 + t2) / 2;
    float pm = (p1 + p2) / 2;

    int idx = ((phi < pm) ? 0 : 1) + 2 * ((theta < tm) ? 0 : 1);
    int c_index = _child_index[idx];

    DInterval cangles;
    cangles._theta[0] = (theta < tm) ? t1 : tm;
    cangles._theta[1] = (theta < tm) ? tm : t2;
    cangles._phi[0] = (phi < pm) ? p1 : pm;
    cangles._phi[1] = (phi < pm) ? pm : p2;

    (this + (c_index - index))->fill(c_index, theta, phi, Li, cangles);
}

void DTree::update(int index, float flux) {
    _flux += flux;
    DTree* root = this + GET_DTREE_ROOT_INDEX(index) - index;

    if (_flux / root->_flux <= __rho) {
        return;
    }

    // no child
    if (_child_index[0] == -1) {
        int idx = get_index(index);

        // no space left
        if (idx == -1) { return; }
        float flux_to_add = _flux / DTREE_CHILD_NODE;
        for (int i = 0; i < DTREE_CHILD_NODE; ++i) {
            _child_index[i] = idx;
            (this + (idx - index))->update(idx, flux_to_add);
            ++idx;
        }
        return;
    }

    for (int i = 0; i < DTREE_CHILD_NODE; ++i) {
        int idx = _child_index[i];
        (this + (idx - index))->update(idx, 0);
    }
}

int DTree::get_root_index(int index) {
    return GET_DTREE_ROOT_INDEX(index);
}


int DTree::get_index(int index) {
    int t_index = GET_DTREE_INDEX(index);
    int& node_index = __node_idx[t_index];
    if (node_index + 4 >= MAX_NODE) {
        return -1;
    }
    int tmp = node_index + 1;
    node_index += 4;
    return t_index * MAX_NODE + tmp;
}


void DTree::initial_split(int index, int depth) {
    if (depth == 0) { return; }
    int idx = get_index(index);
    // should have space left
    assert(idx != -1);

    for (int i = 0; i < DTREE_CHILD_NODE; ++i) {
        _child_index[i] = idx;
        (this + idx - index)->initial_split(idx, depth - 1);
        ++idx;
    }
}

void DTree::print(int index, int depth, DInterval degrees) {
    std::cout << std::string(static_cast<int>(depth << 1), ' ') << index
        << ": flux = " << _flux
        << ", " << degrees
        << std::endl;
    if (_child_index[0] != -1) {
        for (int i = 0; i < DTREE_CHILD_NODE; ++i) {
            // TODO....................
            //degrees ╥жая
            (this + _child_index[i] - index)->print(_child_index[i], depth + 1);
        }
    }
}

int DTree::get_root_index_by_STree_index(int index) {
    return GET_DTREE_ROOT_INDEX_BY_STREE_INDEX(index);
}

void DTree::copy(int src_index, int dst_index) {
    int node_num = __node_idx[src_index];
    __node_idx[dst_index] = node_num;

    assert(__root != nullptr);
    DTree* src_addr = __root + get_root_index_by_STree_index(src_index);
    DTree* dst_addr = __root + get_root_index_by_STree_index(dst_index);
    memcpy(dst_addr, src_addr, sizeof(DTree) * node_num);

    // absolute index
    int MASK = ((1 << DTREE_MAX_NODE_BIT) - 1);
    int MASK2 = get_root_index_by_STree_index(dst_index);
    for (int i = 0; i < node_num; ++i) {
        if (dst_addr->_child_index[0] != -1) {
            for (int j = 0; j < DTREE_CHILD_NODE; ++j) {
                dst_addr->_child_index[j] = (dst_addr->_child_index[j] & MASK) | MASK2;
            }
        }
        ++dst_addr;
    }
}