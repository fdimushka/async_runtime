#include <iostream>
#include "ar/ar.hpp"

namespace AR = AsyncRuntime;

struct Node {
    Node(int v) : val(v) {}
    int val = 0;
    Node *left = nullptr;
    Node *right = nullptr;
};

void dfs(AR::CoroutineHandler* handler, AR::Yield<int> & sink, Node *x) {
    if (x->left) {
        dfs(handler, sink, x->left);
    }
    sink(x->val);
    if (x->right) {
        dfs(handler, sink, x->right);
    }
}

int main() {
    Node *x = new Node(10);
    x->left = new Node(5);
    x->left->left = new Node(3);
    x->left->right = new Node(7);
    x->right = new Node(20);
    x->right->left = new Node(15);
    x->right->left->right = new Node(17);

    auto coro = AR::MakeCoroutine<int>(&dfs, x);

    std::cout << "Tree: \n";

    for(int i : coro) {
        std::cout << i << std::endl;
    }

    return 0;
}

