#include <catch2/catch_test_macros.hpp>

#include "../tools/graph.hpp"

#include <string>
#include <vector>


TEST_CASE("empty graph has no nodes and no edges") {
    graph::orgraph_t<int, int> g;

    REQUIRE(g.nodes().empty());
    REQUIRE(g.edges().empty());
    REQUIRE(g.nodes_begin() == g.nodes_end());
    REQUIRE(g.edges_begin() == g.edges_end());

    REQUIRE_NOTHROW(g.validate());
}


TEST_CASE("add_node creates a node with the given data") {
    graph::orgraph_t<int, int> g;

    auto n = g.add_node(42);

    REQUIRE(g.nodes().size() == 1);
    REQUIRE(g.edges().empty());
    REQUIRE(n->get_data() == 42);
    REQUIRE(n->get_in().empty());
    REQUIRE(n->get_out().empty());

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("add_node can be called many times") {
    graph::orgraph_t<int, int> g;

    std::vector<graph::orgraph_t<int, int>::node_it> nodes;
    for (int i = 0; i < 100; ++i) {
        nodes.push_back(g.add_node(i));
    }

    REQUIRE(g.nodes().size() == 100);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(nodes[i]->get_data() == i);
    }

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("node data can be modified through get_data") {
    graph::orgraph_t<int, int> g;
    auto n = g.add_node(1);

    n->get_data() = 100;
    REQUIRE(n->get_data() == 100);

    const auto& cg = g;
    REQUIRE(cg.nodes().front().get_data() == 100);
}


TEST_CASE("add_edge connects two nodes") {
    graph::orgraph_t<int, std::string> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto e = g.add_edge(a, b, "a->b");

    REQUIRE(g.edges().size() == 1);
    REQUIRE(e->get_data() == "a->b");
    REQUIRE(e->source() == a);
    REQUIRE(e->target() == b);

    REQUIRE(a->get_out().size() == 1);
    REQUIRE(a->get_in().empty());
    REQUIRE(b->get_in().size() == 1);
    REQUIRE(b->get_out().empty());

    REQUIRE(a->get_out().front() == e);
    REQUIRE(b->get_in().front() == e);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("chain of edges") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);
    auto d = g.add_node(4);

    auto e1 = g.add_edge(a, b, 0);
    auto e2 = g.add_edge(b, c, 0);
    auto e3 = g.add_edge(c, d, 0);

    REQUIRE(g.edges().size() == 3);

    REQUIRE(a->get_out().size() == 1);
    REQUIRE(a->get_in().empty());
    REQUIRE(b->get_in().size() == 1);
    REQUIRE(b->get_out().size() == 1);
    REQUIRE(c->get_in().size() == 1);
    REQUIRE(c->get_out().size() == 1);
    REQUIRE(d->get_in().size() == 1);
    REQUIRE(d->get_out().empty());

    REQUIRE(e1->source() == a);
    REQUIRE(e1->target() == b);
    REQUIRE(e3->source() == c);
    REQUIRE(e3->target() == d);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("self loop is allowed and consistent") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto e = g.add_edge(a, a, 0);

    REQUIRE(g.edges().size() == 1);
    REQUIRE(e->source() == a);
    REQUIRE(e->target() == a);
    REQUIRE(a->get_in().size() == 1);
    REQUIRE(a->get_out().size() == 1);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("parallel edges between the same pair of nodes") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto e1 = g.add_edge(a, b, 10);
    auto e2 = g.add_edge(a, b, 20);

    REQUIRE(g.edges().size() == 2);
    REQUIRE(a->get_out().size() == 2);
    REQUIRE(b->get_in().size() == 2);
    REQUIRE(e1->get_data() == 10);
    REQUIRE(e2->get_data() == 20);

    REQUIRE_NOTHROW(g.validate());
}


TEST_CASE("graph with void data types works") {
    graph::orgraph_t<void, void> g;

    auto a = g.add_node();
    auto b = g.add_node();
    auto c = g.add_node();

    auto e1 = g.add_edge(a, b);
    auto e2 = g.add_edge(b, c);

    REQUIRE(g.nodes().size() == 3);
    REQUIRE(g.edges().size() == 2);
    REQUIRE(e1->source() == a);
    REQUIRE(e1->target() == b);
    REQUIRE(e2->source() == b);
    REQUIRE(e2->target() == c);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("node-only void data, edge has data") {
    graph::orgraph_t<void, std::string> g;

    auto a = g.add_node();
    auto b = g.add_node();
    auto e = g.add_edge(a, b, "hello");

    REQUIRE(e->get_data() == "hello");
    REQUIRE_NOTHROW(g.validate());
}


TEST_CASE("iterators to nodes remain valid after adding more nodes") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);

    for (int i = 0; i < 50; ++i) {
        g.add_node(100 + i);
    }

    REQUIRE(a->get_data() == 1);
    REQUIRE(b->get_data() == 2);
}

TEST_CASE("iterators to edges remain valid after adding more edges") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);
    auto d = g.add_node(4);

    auto e1 = g.add_edge(a, b, 0);

    for (int i = 0; i < 50; ++i) {
        g.add_edge(c, d, i);
    }

    REQUIRE(e1->source() == a);
    REQUIRE(e1->target() == b);
    REQUIRE(e1->get_data() == 0);
}


TEST_CASE("set_source moves an edge to a new source") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);
    auto e = g.add_edge(a, b, 0);

    g.set_source(e, c);

    REQUIRE(e->source() == c);
    REQUIRE(e->target() == b);

    REQUIRE(a->get_out().empty());
    REQUIRE(c->get_out().size() == 1);
    REQUIRE(c->get_out().front() == e);

    REQUIRE(b->get_in().size() == 1);
    REQUIRE(b->get_in().front() == e);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("set_target moves an edge to a new target") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);
    auto e = g.add_edge(a, b, 0);

    g.set_target(e, c);

    REQUIRE(e->source() == a);
    REQUIRE(e->target() == c);

    REQUIRE(b->get_in().empty());
    REQUIRE(c->get_in().size() == 1);
    REQUIRE(c->get_in().front() == e);

    REQUIRE(a->get_out().size() == 1);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("set_source can create a self loop") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto e = g.add_edge(a, b, 0);

    g.set_source(e, b);

    REQUIRE(e->source() == b);
    REQUIRE(e->target() == b);
    REQUIRE(b->get_in().size() == 1);
    REQUIRE(b->get_out().size() == 1);
    REQUIRE(a->get_out().empty());

    REQUIRE_NOTHROW(g.validate());
}



TEST_CASE("erase_edge removes only the edge") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto e = g.add_edge(a, b, 0);

    g.erase_edge(e);

    REQUIRE(g.edges().empty());
    REQUIRE(g.nodes().size() == 2);
    REQUIRE(a->get_out().empty());
    REQUIRE(b->get_in().empty());

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("erase_edge removes the right edge among many") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);

    auto e1 = g.add_edge(a, b, 10);
    auto e2 = g.add_edge(b, c, 20);
    auto e3 = g.add_edge(a, c, 30);

    g.erase_edge(e2);

    REQUIRE(g.edges().size() == 2);
    REQUIRE(a->get_out().size() == 2);
    REQUIRE(b->get_in().size() == 1);
    REQUIRE(b->get_out().size() == 0);
    REQUIRE(c->get_in().size() == 1);

    REQUIRE(e1->get_data() == 10);
    REQUIRE(e3->get_data() == 30);

    REQUIRE_NOTHROW(g.validate());
}


TEST_CASE("erase_node without edges just removes the node") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);

    g.erase_node(a);

    REQUIRE(g.nodes().size() == 1);
    REQUIRE(g.edges().empty());

    REQUIRE(b->get_data() == 2);
    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("erase_node removes all incident edges") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);


    g.add_edge(a, b, 0);
    g.add_edge(b, c, 0);
    g.add_edge(a, c, 0);

    g.erase_node(b);

    REQUIRE(g.nodes().size() == 2);
    REQUIRE(g.edges().size() == 1);

    REQUIRE(a->get_out().size() == 1);
    REQUIRE(c->get_in().size() == 1);

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("erase_node with self loop") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);

    g.add_edge(a, a, 0);
    g.add_edge(a, b, 0);
    g.add_edge(b, a, 0);

    g.erase_node(a);

    REQUIRE(g.nodes().size() == 1);
    REQUIRE(g.edges().empty());

    REQUIRE(b->get_in().empty());
    REQUIRE(b->get_out().empty());

    REQUIRE_NOTHROW(g.validate());
}

TEST_CASE("erase all nodes one by one") {
    graph::orgraph_t<int, int> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);

    g.add_edge(a, b, 0);
    g.add_edge(b, c, 0);
    g.add_edge(c, a, 0);

    g.erase_node(a);
    REQUIRE(g.nodes().size() == 2);
    REQUIRE(g.edges().size() == 1);

    g.erase_node(b);
    REQUIRE(g.nodes().size() == 1);
    REQUIRE(g.edges().empty());

    g.erase_node(c);
    REQUIRE(g.nodes().empty());
    REQUIRE(g.edges().empty());

    REQUIRE_NOTHROW(g.validate());
}



TEST_CASE("complex scenario: build, modify, tear down") {
    graph::orgraph_t<int, std::string> g;

    auto a = g.add_node(1);
    auto b = g.add_node(2);
    auto c = g.add_node(3);

    auto ab = g.add_edge(a, b, "ab");
    auto bc = g.add_edge(b, c, "bc");
    auto ca = g.add_edge(c, a, "ca");
    auto ac = g.add_edge(a, c, "ac");

    REQUIRE(g.nodes().size() == 3);
    REQUIRE(g.edges().size() == 4);
    REQUIRE_NOTHROW(g.validate());

    g.set_source(ac, b);
    REQUIRE(ac->source() == b);
    REQUIRE(b->get_out().size() == 2);
    REQUIRE(a->get_out().size() == 1);
    REQUIRE_NOTHROW(g.validate());

    g.erase_edge(ab);
    REQUIRE(g.edges().size() == 3);
    REQUIRE(a->get_out().size() == 0);
    REQUIRE_NOTHROW(g.validate());

    g.erase_node(b);
    REQUIRE(g.nodes().size() == 2);
    REQUIRE(g.edges().size() == 1);
    REQUIRE_NOTHROW(g.validate());

    REQUIRE(ca->target() == a);
}

TEST_CASE("data survives erase of unrelated elements") {
    graph::orgraph_t<std::string, std::string> g;

    auto a = g.add_node("alpha");
    auto b = g.add_node("beta");
    auto c = g.add_node("gamma");

    auto e1 = g.add_edge(a, b, "e1");
    auto e2 = g.add_edge(b, c, "e2");

    g.erase_edge(e1);
    g.erase_node(b);

    REQUIRE(g.nodes().size() == 2);
    REQUIRE(g.edges().empty());

    REQUIRE(a->get_data() == "alpha");
    REQUIRE(c->get_data() == "gamma");
}