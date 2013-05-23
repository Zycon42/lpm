/**
 * @file bintrie.h
 *
 * @author Jan Dušek <xdusek17@stud.fit.vutbr.cz>
 * @date 2013
 *
 * File with class BinaryTrie
 */

#ifndef BINTRIE_H
#define	BINTRIE_H

#include "bitarray.h"

#include <iostream>

template <size_t N, class T>
class BinaryTrie
{
public:
    typedef T mapped_type;
    typedef BitArray<N> key_type;
    //typedef std::pair<const key_type, mapped_type> value_type;

    BinaryTrie() : root(NULL), numNodes(0) { }

    ~BinaryTrie() {
        clear();
    }

    void clear() {
        if (root) {
            Node* stack[N * 8 + 1];
            Node** sp = stack;
            Node* node = root;

            while (node) {
                Node* right = node->right;
                Node* left = node->left;

                delete node;

                if (left) {
                    if (right)
                        *sp++ = right;
                    node = left;
                } else if (right) {
                    node = right;
                } else if (sp != stack) {
                    node = *(--sp);
                } else {
                    node = NULL;
                }
            }

            numNodes = 0;
        }
    }

    bool empty() const {
        return root == NULL;
    }

    size_t size() const {
        return numNodes;
    }

    mapped_type& operator[](const key_type& key) {
        Node* node = lookupNode(key);
        if (node)
            return node->data;
        else
            throw std::out_of_range("BinaryTrie::operator[]: failed inserting key into trie");
    }

    mapped_type& at(const key_type& key) {
        Node* node = searchExact(key);
        if (node)
            return node->data;
        else
            throw std::out_of_range("BinaryTrie::at: no such key in trie");
    }

    mapped_type& best(const key_type& key) {
        Node* node = searchBest(key);
        if (node)
            return node->data;
        else
            throw std::out_of_range("BinaryTrie::best: no prefixes in trie for given key");
    }

    void erase(const key_type& key) {
        Node* node = searchExact(key);
        if (node)
            removeNode(node);
        else
            throw std::out_of_range("BinaryTrie::erase: no such key in trie");
    }
private:
    BinaryTrie(const BinaryTrie&);
    BinaryTrie& operator=(const BinaryTrie&);

    struct Node
    {
        Node() : left(NULL), right(NULL), parent(NULL), bits(0) { }
        Node(const key_type& key, size_t bits) : left(NULL), right(NULL), parent(NULL), key(key), bits(bits) { }

        Node *left, *right, *parent;
        mapped_type data;
        key_type key;
        size_t bits;                        // number of bits in key that's valid for this node
    };

    Node* lookupNode(const key_type& key);
    Node* searchExact(const key_type& key);
    Node* searchBest(const key_type& key);
    void removeNode(Node* node);

    Node* root;
    size_t numNodes;
};

template <size_t N, class T>
typename BinaryTrie<N,T>::Node* BinaryTrie<N,T>::lookupNode(const typename BinaryTrie<N,T>::key_type& key) {
    // if we don't have root yet create it.
    if (root == NULL) {
        Node* node = new BinaryTrie<N,T>::Node(key, key.size());
        root = node;
        numNodes++;
        return node;
    }

    // walk to nearest data node. note that leafs are always data nodes.
    Node* node = root;
    size_t bitLen = key.size();
    while (node->bits < bitLen || node->key.empty()) {
        if (key[node->bits]) {
            if (node->right == NULL)
                break;
            node = node->right;
        } else {
            if (node->left == NULL)
                break;
            node = node->left;
        }
    }
    // store node key, which we compare with given key
    key_type tmpKey = node->key;

    // find first different bit
    size_t checkBit = (node->bits < bitLen) ? node->bits : bitLen;
    size_t diffBit = key.firstDifferentBit(node->key, checkBit);

    // walk back before different bit
    Node* parent = node->parent;
    while (parent && parent->bits >= diffBit) {
        node = parent;
        parent = node->parent;
    }

    // if we are on right node return it
    if (diffBit == bitLen && node->bits == bitLen) {
        // if this was glue, set prefix
        if (node->key.empty())
            node->key = key;
        return node;
    }

    // create new node
    Node* newNode = new BinaryTrie<N,T>::Node(key, key.size());
    numNodes++;

    if (node->bits == diffBit) {
        // put newNode after current node
        newNode->parent = node;
        if (key[node->bits])
            node->right = newNode;
        else
            node->left = newNode;
        return newNode;
    }

    if (bitLen == diffBit) {
        // put newNode before current node

        if (tmpKey[bitLen])
            newNode->right = node;
        else
            newNode->left = node;

        newNode->parent = node->parent;
        if (node->parent == NULL)
            root = newNode;
        else if (node->parent->right == node)
            node->parent->right = newNode;
        else
            node->parent->left = newNode;

        node->parent = newNode;
    } else {
        // put newNode next to current node

        // create glue node
        Node* glue = new BinaryTrie<N,T>::Node();
        glue->bits = diffBit;
        glue->parent = node->parent;
        numNodes++;

        if (key[diffBit]) {
            glue->right = newNode;
            glue->left = node;
        } else {
            glue->right = node;
            glue->left = newNode;
        }

        newNode->parent = glue;

        if (node->parent == NULL)
            root = glue;
        else if (node->parent->right == node)
            node->parent->right = glue;
        else
            node->parent->left = glue;

        node->parent = glue;
    }

    return newNode;
}

template <size_t N, class T>
typename BinaryTrie<N,T>::Node* BinaryTrie<N,T>::searchExact(const typename BinaryTrie<N,T>::key_type& key) {
    // on empty trie don't bother
    if (root == NULL)
        return NULL;

    // walk trie until we find node with key size >= given key size
    Node* node = root;
    while (node->bits < key.size()) {
        if (key[node->bits])
            node = node->right;
        else
            node = node->left;

        // on trie bottom we know that node with given key doesn't exists
        if (node == NULL)
            return NULL;
    }

    // node must have right key size and must be data node
    if (node->bits > key.size() || node->key.empty())
        return NULL;

    if (key.compareBits(node->key, key.size()))
        return node;

    return NULL;
}

template <size_t N, class T>
typename BinaryTrie<N,T>::Node* BinaryTrie<N,T>::searchBest(const typename BinaryTrie<N,T>::key_type& key) {
    // on empty trie don't bother
    if (root == NULL)
        return NULL;

    Node* stack[N * 8 + 1];     // stack for storing nodes that are prefix of given key
    int cnt = 0;            // stack counter

    // walk trie until we find node with key size >= given key size or hit bottom
    Node* node = root;
    while (node->bits < key.size()) {
        // store data nodes to stack
        if (!node->key.empty())
            stack[cnt++] = node;

        if (key[node->bits])
            node = node->right;
        else
            node = node->left;

        if (node == NULL)
            break;
    }

    // store also current data node
    if (node && !node->key.empty())
        stack[cnt++] = node;

    // go through stack are test keys if they are equal. cuz its stack
    // longest matching prefix will be found.
    while (--cnt >= 0) {
        node = stack[cnt];
        if (key.compareBits(node->key, node->key.size()) && node->key.size() <= key.size())
            return node;
    }

    return NULL;
}

template <size_t N, class T>
void BinaryTrie<N,T>::removeNode(typename BinaryTrie<N,T>::Node* node) {
    // if node has children
    if (node->right && node->left) {
        node->key = key_type();     // set key to empty, this will indicate that node isn't data node
        return;
    }
    // when node is leaf
    if (node->right == NULL && node->left == NULL) {
        Node* parent = node->parent;
        delete node;
        numNodes--;

        // this was last node
        if (parent == NULL) {
            root = NULL;
            delete node;
            return;
        }

        // get another parent child, aka node's sibling
        Node* child;
        if (parent->right == node) {
            parent->right = NULL;
            child = parent->left;
        } else {
            parent->left = NULL;
            child = parent->right;
        }

        // if parent is data node we are finished
        if (!parent->key.empty())
            return;

        // otherwise we need to delete parent too
        if (parent->parent == NULL)
            root = child;
        else if (parent->parent->right == parent)
            parent->parent->right = child;
        else
            parent->parent->left = child;

        child->parent = parent->parent;
        delete parent;
        numNodes--;
        return;
    }

    // node has 1 child

    // get child
    Node* child;
    if (node->right)
        child = node->right;
    else
        child = node->left;

    // link parent
    Node* parent = node->parent;
    child->parent = parent;

    delete node;
    numNodes--;

    if (parent == NULL) {
        root = child;
        return;
    }
    if (parent->right == node)
        parent->right = child;
    else
        parent->left = child;
}

#endif	/* BINTRIE_H */

