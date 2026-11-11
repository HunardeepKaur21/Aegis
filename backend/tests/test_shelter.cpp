#include "shelter/victim_types.hpp"
#include "shelter/victim_linked_list.hpp"
#include "shelter/victim_bst.hpp"
#include "shelter/victim_avl.hpp"
#include "shelter/victim_hash_map.hpp"
#include "shelter/shelter_recommender.hpp"
#include "graphs/graph.hpp"
#include <iostream>
#include <vector>
#include <string>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

using namespace aegis;

static void testLinkedLists() {
    // 1. Singly Linked List
    SinglyLinkedList<int> sList;
    CHECK(sList.empty());
    sList.insertTail(10);
    sList.insertTail(20);
    sList.insertHead(5);
    sList.insertAt(2, 15); // [5, 10, 15, 20]

    auto v1 = sList.toVector();
    CHECK(v1.size() == 4);
    CHECK(v1[0] == 5 && v1[1] == 10 && v1[2] == 15 && v1[3] == 20);

    CHECK(sList.deleteNode([](int x) { return x == 10; }));
    auto v2 = sList.toVector(); // [5, 15, 20]
    CHECK(v2.size() == 3);
    CHECK(v2[1] == 15);

    sList.reverse(); // [20, 15, 5]
    auto v3 = sList.toVector();
    CHECK(v3[0] == 20 && v3[1] == 15 && v3[2] == 5);

    // 2. Doubly Linked List
    DoublyLinkedList<std::string> dList;
    dList.insertTail("Manjit");
    dList.insertTail("Arjun");
    dList.insertHead("Priya"); // [Priya, Manjit, Arjun]
    CHECK(dList.size() == 3);

    dList.deleteNode([](const std::string& s) { return s == "Manjit"; });
    auto dv1 = dList.toVector(); // [Priya, Arjun]
    CHECK(dv1.size() == 2);
    CHECK(dv1[0] == "Priya" && dv1[1] == "Arjun");

    dList.reverse(); // [Arjun, Priya]
    auto dv2 = dList.toVector();
    CHECK(dv2[0] == "Arjun" && dv2[1] == "Priya");
}

static void testBST() {
    BinarySearchTree<int, std::string> bst;
    bst.insert(50, "Root");
    bst.insert(30, "Left");
    bst.insert(70, "Right");
    bst.insert(20, "Left-Left");
    bst.insert(40, "Left-Right");
    bst.insert(60, "Right-Left");
    bst.insert(80, "Right-Right");

    CHECK(bst.size() == 7);
    CHECK(bst.search(50) != nullptr && *bst.search(50) == "Root");
    CHECK(bst.search(20) != nullptr && *bst.search(20) == "Left-Left");
    CHECK(bst.search(99) == nullptr);

    // In-order traversal must be sorted: 20, 30, 40, 50, 60, 70, 80
    auto inOrder = bst.inorderTraversal();
    CHECK(inOrder.size() == 7);
    CHECK(inOrder[0] == "Left-Left");
    CHECK(inOrder[3] == "Root");
    CHECK(inOrder[6] == "Right-Right");

    // Remove node with two children (30)
    CHECK(bst.remove(30));
    CHECK(bst.size() == 6);
    CHECK(bst.search(30) == nullptr);
    CHECK(bst.search(20) != nullptr);
    CHECK(bst.search(40) != nullptr);
}

static void testAVL() {
    AVLTree<int, std::string> avl;
    // Sequential insertion 10, 20, 30, 40, 50 would produce an unbalanced chain of height 5 in plain BST.
    // In AVL Tree, rotations keep height <= 3!
    avl.insert(10, "Ten");
    avl.insert(20, "Twenty");
    avl.insert(30, "Thirty"); // Triggers RR rotation (left rotation around 10)
    avl.insert(40, "Forty");
    avl.insert(50, "Fifty");  // Triggers RR rotation

    CHECK(avl.size() == 5);
    CHECK(avl.height() <= 3);

    // LR and RL triggers
    avl.insert(25, "Twenty-Five");
    CHECK(avl.size() == 6);
    CHECK(avl.height() <= 4);

    CHECK(avl.search(30) != nullptr && *avl.search(30) == "Thirty");
    CHECK(avl.search(25) != nullptr && *avl.search(25) == "Twenty-Five");
    CHECK(avl.search(999) == nullptr);

    // Deletion maintaining AVL balance
    CHECK(avl.remove(30));
    CHECK(avl.size() == 5);
    CHECK(avl.search(30) == nullptr);
    CHECK(avl.height() <= 3);
}

static void testHashMap() {
    ChainingHashMap<int, std::string> map(4); // Small initial bucket count to test collisions and resizing
    map.insert(1, "Alpha");
    map.insert(5, "Beta");   // Collides with 1 (mod 4 == 1)
    map.insert(9, "Gamma");  // Collides with 1 and 5
    map.insert(2, "Delta");

    CHECK(map.size() == 4);
    CHECK(map.find(1) != nullptr && *map.find(1) == "Alpha");
    CHECK(map.find(5) != nullptr && *map.find(5) == "Beta");
    CHECK(map.find(9) != nullptr && *map.find(9) == "Gamma");
    CHECK(map.find(2) != nullptr && *map.find(2) == "Delta");
    CHECK(map.find(99) == nullptr);

    // Update existing key
    map.insert(5, "Beta-Updated");
    CHECK(map.size() == 4);
    CHECK(*map.find(5) == "Beta-Updated");

    // Remove key from middle of chain
    CHECK(map.remove(5));
    CHECK(map.size() == 3);
    CHECK(map.find(5) == nullptr);
    CHECK(map.find(1) != nullptr && map.find(9) != nullptr);

    // Insert more items to trigger rehash
    for (int i = 100; i < 120; ++i) {
        map.insert(i, "Val" + std::to_string(i));
    }
    CHECK(map.size() == 23);
    CHECK(map.bucketCount() > 4); // Rehashed
    CHECK(map.find(105) != nullptr);
}

static void testShelterRecommendation() {
    // Setup simple road graph:
    // Location 1 (Victim) --(2.0 km)--> Location 2 (Shelter A, full)
    // Location 1 (Victim) --(5.0 km)--> Location 3 (Shelter B, available, medical)
    // Location 1 (Victim) --(8.0 km)--> Location 4 (Shelter C, available, no medical)
    // Location 5 (Disconnected)
    Graph g;
    g.addNode(1); g.addNode(2); g.addNode(3); g.addNode(4); g.addNode(5);
    g.addEdge(1, 2, 2.0, 101, false);
    g.addEdge(1, 3, 5.0, 102, false);
    g.addEdge(1, 4, 8.0, 103, false);

    std::map<int, std::string> locNames = {
        {1, "Riverbank"}, {2, "School"}, {3, "Hospital Grounds"}, {4, "Sports Complex"}, {5, "Island"}
    };

    VictimRecord victim;
    victim.id = 1;
    victim.name = "Test Victim";
    victim.current_location_id = 1;
    victim.accompanying_people = 1; // Needs 2 spots
    victim.medical_assistance = true;

    std::vector<ShelterInfo> shelters = {
        {1, "Shelter A", 2, "School", 100, 100, 0, false, "Beds", "Full", 100.0},
        {2, "Shelter B", 3, "Hospital Grounds", 300, 100, 200, true, "Medical Bay", "Available", 33.3},
        {3, "Shelter C", 4, "Sports Complex", 500, 200, 300, false, "Cots", "Available", 40.0}
    };

    auto res = ShelterRecommender::recommendShelter(victim, shelters, g, locNames);
    CHECK(res.found);
    // Shelter A should be excluded because it's Full.
    // Shelter B has medical facility and distance 5.0 km.
    CHECK(res.primary_recommendation.shelter.name == "Shelter B");
    CHECK(res.primary_recommendation.distance_km == 5.0);
    CHECK(res.alternatives.size() == 1);
    CHECK(res.alternatives[0].shelter.name == "Shelter C");

    // Test victim in disconnected location
    VictimRecord discVictim = victim;
    discVictim.current_location_id = 5;
    auto discRes = ShelterRecommender::recommendShelter(discVictim, shelters, g, locNames);
    CHECK(!discRes.found);
}

int main() {
    std::cout << "Running Shelter & Victim DSA Unit Tests...\n";
    testLinkedLists();
    testBST();
    testAVL();
    testHashMap();
    testShelterRecommendation();

    if (failures == 0) {
        std::cout << "All Shelter & Victim tests passed successfully!\n";
        return 0;
    } else {
        std::cerr << failures << " tests failed!\n";
        return 1;
    }
}
