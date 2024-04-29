#include <iostream>
#include "ar/ar.hpp"

using namespace AsyncRuntime;

struct Node {
    Node(int v) : val(v) {}
    int val = 0;
    Node *left = nullptr;
    Node *right = nullptr;
};

int dfs(coroutine_handler* handler, yield<int> & sink, Node *x) {
    if (x->left) {
        dfs(handler, sink, x->left);
    }
    sink(x->val);
    if (x->right) {
        dfs(handler, sink, x->right);
    }

    return 0;
}

int main() {
    Node *x = new Node(10);
    x->left = new Node(5);
    x->left->left = new Node(3);
    x->left->right = new Node(7);
    x->right = new Node(20);
    x->right->left = new Node(15);
    x->right->left->right = new Node(17);

    auto coro_fib = make_coroutine<int>(&dfs, x);
    std::cout << "Tree: \n";

    while (!coro_fib->is_completed()) {
        coro_fib->init_promise();
        auto f = coro_fib->get_future();
        coro_fib->resume();
        std::cout << f.get() << std::endl;
    }

    return 0;
}

