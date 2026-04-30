/*
Student 1: YOUR NAME
Student 2: YOUR PARTNER NAME (if applicable)

Project: Smart Traffic Simulation System (CSE 1101 Assignment 2)

Description:
This program simulates traffic flow in a small road network using core data structures
implemented manually (without STL queue/map/list).

Hash Function:
- The vehicle registry hash table uses: index = id % TABLE_SIZE.
- This spreads IDs across a fixed number of buckets.

Collision Handling (Separate Chaining):
- If multiple vehicle IDs map to the same bucket index, they are stored in a singly linked list
  in that bucket.
- Insert adds a new node at the beginning of the linked list.

Queue Handling Strategy:
- Each intersection uses a manually implemented linked-list queue.
- FCFS strategy removes from front.
- Priority strategy scans for first ambulance and removes it first; otherwise FCFS.

Road Network Representation:
- Each intersection stores a linked list of connected intersections.
- Network used: A->B, A->C, B->D, C->D, and D is exit.

Traffic Strategies:
- FCFS: first-come-first-serve queue order.
- PRIORITY: first ambulance in queue goes first; if none, normal dequeue.

Simulation Logic:
- Vehicles are generated automatically at startup.
- Vehicles start at intersection A.
- Each simulation step processes each intersection once, moves vehicles to next connection
  (round-robin choice) or exits at D.
- Exited vehicles are recorded in a fixed-size circular history list.

Assumptions and Limitations:
- Vehicle generation is random but deterministic enough for demonstration purposes.
- One vehicle per intersection is processed per step.
- This is a teaching project, so logic is intentionally simple and readable.
*/

#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace std;

// Vehicle entity stored in queues, hash table, and history.
struct Vehicle {
    int id;
    string licensePlate;
    string vehicleType;
    int arrivalTime;
};

// Node for separate chaining in hash table.
struct HashNode {
    Vehicle* vehicle;
    HashNode* next;
    HashNode(Vehicle* v) : vehicle(v), next(NULL) {}
};

// Custom hash table keyed by vehicle ID.
class VehicleHashTable {
private:
    static const int TABLE_SIZE = 23;
    HashNode* buckets[TABLE_SIZE];

    int hashFunction(int id) {
        return id % TABLE_SIZE;
    }

public:
    VehicleHashTable() {
        for (int i = 0; i < TABLE_SIZE; i++) {
            buckets[i] = NULL;
        }
    }

    void insert(Vehicle* v) {
        int idx = hashFunction(v->id);
        HashNode* node = new HashNode(v);
        node->next = buckets[idx];
        buckets[idx] = node;
    }

    bool remove(int id) {
        int idx = hashFunction(id);
        HashNode* cur = buckets[idx];
        HashNode* prev = NULL;

        while (cur != NULL) {
            if (cur->vehicle->id == id) {
                if (prev == NULL) {
                    buckets[idx] = cur->next;
                } else {
                    prev->next = cur->next;
                }
                delete cur;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    Vehicle* lookup(int id) {
        int idx = hashFunction(id);
        HashNode* cur = buckets[idx];

        while (cur != NULL) {
            if (cur->vehicle->id == id) {
                return cur->vehicle;
            }
            cur = cur->next;
        }
        return NULL;
    }

    void printRegistry() {
        cout << "\n--- Vehicle Registry (Hash Table) ---\n";
        for (int i = 0; i < TABLE_SIZE; i++) {
            cout << "Bucket " << i << ": ";
            HashNode* cur = buckets[i];
            if (cur == NULL) {
                cout << "empty";
            }
            while (cur != NULL) {
                cout << "[ID:" << cur->vehicle->id << ", " << cur->vehicle->vehicleType << "] ";
                cur = cur->next;
            }
            cout << "\n";
        }
    }
};

// Node for queue implementation.
struct QueueNode {
    Vehicle* vehicle;
    QueueNode* next;
    QueueNode(Vehicle* v) : vehicle(v), next(NULL) {}
};

// Linked-list queue for each intersection.
class VehicleQueue {
private:
    QueueNode* front;
    QueueNode* rear;

public:
    VehicleQueue() : front(NULL), rear(NULL) {}

    void enqueue(Vehicle* v) {
        QueueNode* node = new QueueNode(v);
        if (rear == NULL) {
            front = rear = node;
        } else {
            rear->next = node;
            rear = node;
        }
    }

    Vehicle* dequeue() {
        if (front == NULL) return NULL;

        QueueNode* temp = front;
        Vehicle* v = temp->vehicle;
        front = front->next;
        if (front == NULL) rear = NULL;
        delete temp;
        return v;
    }

    Vehicle* peek() {
        if (front == NULL) return NULL;
        return front->vehicle;
    }

    bool isEmpty() {
        return front == NULL;
    }

    // Removes first ambulance for priority strategy; fallback to dequeue.
    Vehicle* dequeueFirstAmbulance() {
        if (front == NULL) return NULL;

        QueueNode* cur = front;
        QueueNode* prev = NULL;

        while (cur != NULL) {
            if (cur->vehicle->vehicleType == "ambulance") {
                Vehicle* v = cur->vehicle;
                if (prev == NULL) {
                    front = cur->next;
                } else {
                    prev->next = cur->next;
                }
                if (cur == rear) {
                    rear = prev;
                }
                delete cur;
                return v;
            }
            prev = cur;
            cur = cur->next;
        }

        return dequeue();
    }

    void printQueue() {
        QueueNode* cur = front;
        if (cur == NULL) {
            cout << "(empty)";
            return;
        }
        while (cur != NULL) {
            cout << "[ID:" << cur->vehicle->id << ", " << cur->vehicle->vehicleType << "] ";
            cur = cur->next;
        }
    }

    void printQueueToFile(ofstream& out) {
        QueueNode* cur = front;
        if (cur == NULL) {
            out << "(empty)";
            return;
        }
        while (cur != NULL) {
            out << "[ID:" << cur->vehicle->id << ", " << cur->vehicle->vehicleType << "] ";
            cur = cur->next;
        }
    }
};

class Intersection;

// Node for adjacency/connection list.
struct ConnectionNode {
    Intersection* intersection;
    ConnectionNode* next;
    ConnectionNode(Intersection* i) : intersection(i), next(NULL) {}
};

// Road intersection with queue and outgoing connections.
class Intersection {
public:
    string name;
    VehicleQueue queue;
    ConnectionNode* connections;
    int rotateIndex;

    Intersection(string n) : name(n), connections(NULL), rotateIndex(0) {}

    void addConnection(Intersection* target) {
        ConnectionNode* node = new ConnectionNode(target);
        if (connections == NULL) {
            connections = node;
        } else {
            ConnectionNode* cur = connections;
            while (cur->next != NULL) cur = cur->next;
            cur->next = node;
        }
    }

    Intersection* getNextConnectionRoundRobin() {
        if (connections == NULL) return NULL;

        int count = 0;
        ConnectionNode* cur = connections;
        while (cur != NULL) {
            count++;
            cur = cur->next;
        }

        int idx = rotateIndex % count;
        rotateIndex++;

        cur = connections;
        for (int i = 0; i < idx; i++) {
            cur = cur->next;
        }
        return cur->intersection;
    }
};

// Node in circular history list.
struct HistoryNode {
    Vehicle* vehicle;
    HistoryNode* next;
    HistoryNode(Vehicle* v) : vehicle(v), next(NULL) {}
};

// Fixed-capacity circular history list.
class TrafficHistory {
private:
    HistoryNode* head;
    HistoryNode* tail;
    int size;
    int capacity;

public:
    TrafficHistory(int cap) : head(NULL), tail(NULL), size(0), capacity(cap) {}

    void addToHistory(Vehicle* v) {
        if (size < capacity) {
            HistoryNode* node = new HistoryNode(v);
            if (head == NULL) {
                head = tail = node;
                node->next = head;
            } else {
                node->next = head;
                tail->next = node;
                tail = node;
            }
            size++;
        } else {
            // Overwrite oldest by moving head pointer and replacing its vehicle.
            head->vehicle = v;
            head = head->next;
            tail = tail->next;
        }
    }

    void printHistory() {
        cout << "\n--- Recent Traffic History (Most recent circular window) ---\n";
        if (head == NULL) {
            cout << "No history.\n";
            return;
        }

        HistoryNode* cur = head;
        for (int i = 0; i < size; i++) {
            cout << i + 1 << ". ID:" << cur->vehicle->id
                 << " Plate:" << cur->vehicle->licensePlate
                 << " Type:" << cur->vehicle->vehicleType
                 << " Arrival:" << cur->vehicle->arrivalTime << "\n";
            cur = cur->next;
        }
    }

    void saveToFile(const string& fileName) {
        ofstream out(fileName.c_str());
        if (!out.is_open()) return;

        out << "Recent processed vehicles (circular history, max " << capacity << ")\n";
        if (head == NULL) {
            out << "No history.\n";
        } else {
            HistoryNode* cur = head;
            for (int i = 0; i < size; i++) {
                out << i + 1 << ". ID:" << cur->vehicle->id
                    << " Plate:" << cur->vehicle->licensePlate
                    << " Type:" << cur->vehicle->vehicleType
                    << " Arrival:" << cur->vehicle->arrivalTime << "\n";
                cur = cur->next;
            }
        }
        out.close();
    }
};

enum Strategy { FCFS = 1, PRIORITY = 2 };

// Controls intersections, vehicle movement, and simulation execution.
class TrafficSimulation {
private:
    Intersection* A;
    Intersection* B;
    Intersection* C;
    Intersection* D;

public:
    TrafficHistory history;

    TrafficSimulation() : history(10) {
        A = new Intersection("A");
        B = new Intersection("B");
        C = new Intersection("C");
        D = new Intersection("D");

        A->addConnection(B);
        A->addConnection(C);
        B->addConnection(D);
        C->addConnection(D);
    }

    Intersection* getA() { return A; }
    Intersection* getB() { return B; }
    Intersection* getC() { return C; }
    Intersection* getD() { return D; }

    void processOneIntersection(Intersection* inter, Strategy strategy) {
        if (inter->queue.isEmpty()) {
            cout << "Intersection " << inter->name << ": no vehicle waiting.\n";
            return;
        }

        Vehicle* moving = NULL;
        if (strategy == PRIORITY) moving = inter->queue.dequeueFirstAmbulance();
        else moving = inter->queue.dequeue();

        if (moving == NULL) {
            cout << "Intersection " << inter->name << ": dequeue returned NULL.\n";
            return;
        }

        cout << "Traffic light GREEN at " << inter->name
             << ": Vehicle ID " << moving->id
             << " (" << moving->vehicleType << ") passes.\n";

        if (inter->name == "D") {
            cout << "Vehicle ID " << moving->id << " exits system from D.\n";
            history.addToHistory(moving);
            return;
        }

        Intersection* next = inter->getNextConnectionRoundRobin();
        if (next != NULL) {
            next->queue.enqueue(moving);
            cout << "Vehicle ID " << moving->id << " moves to intersection " << next->name << ".\n";
        } else {
            cout << "Vehicle ID " << moving->id << " has no next road; exits.\n";
            history.addToHistory(moving);
        }
    }

    void simulateTraffic(int steps, Strategy strategy) {
        cout << "\n==============================\n";
        cout << "Simulation started. Strategy: " << (strategy == FCFS ? "FCFS" : "PRIORITY") << "\n";
        cout << "==============================\n";

        for (int step = 1; step <= steps; step++) {
            cout << "\n--- Step " << step << " ---\n";
            processOneIntersection(A, strategy);
            processOneIntersection(B, strategy);
            processOneIntersection(C, strategy);
            processOneIntersection(D, strategy);

            cout << "Queue A: "; A->queue.printQueue(); cout << "\n";
            cout << "Queue B: "; B->queue.printQueue(); cout << "\n";
            cout << "Queue C: "; C->queue.printQueue(); cout << "\n";
            cout << "Queue D: "; D->queue.printQueue(); cout << "\n";
        }
    }

    void saveQueuesToFile(const string& fileName) {
        ofstream out(fileName.c_str());
        if (!out.is_open()) return;

        out << "Current intersection queues\n";
        out << "A: "; A->queue.printQueueToFile(out); out << "\n";
        out << "B: "; B->queue.printQueueToFile(out); out << "\n";
        out << "C: "; C->queue.printQueueToFile(out); out << "\n";
        out << "D: "; D->queue.printQueueToFile(out); out << "\n";

        out.close();
    }
};

int main() {
    srand((unsigned int)time(NULL));

    const int VEHICLE_COUNT = 50;
    Vehicle* vehicles[VEHICLE_COUNT];

    // Generate vehicles automatically (acceptable per assignment requirement).
    for (int i = 0; i < VEHICLE_COUNT; i++) {
        vehicles[i] = new Vehicle();
        vehicles[i]->id = 1000 + i;
        vehicles[i]->licensePlate = "PLATE" + to_string(100 + i);

        int r = rand() % 10;
        if (r < 2) vehicles[i]->vehicleType = "ambulance";
        else if (r < 5) vehicles[i]->vehicleType = "truck";
        else vehicles[i]->vehicleType = "car";

        vehicles[i]->arrivalTime = i;
    }

    VehicleHashTable registry;

    cout << "Inserting vehicles into hash table...\n";
    for (int i = 0; i < VEHICLE_COUNT; i++) {
        registry.insert(vehicles[i]);
    }
    registry.printRegistry();

    cout << "\nTesting lookup for ID 1005:\n";
    Vehicle* found = registry.lookup(1005);
    if (found != NULL) {
        cout << "Found vehicle -> ID:" << found->id << " Plate:" << found->licensePlate
             << " Type:" << found->vehicleType << "\n";
    } else {
        cout << "Vehicle not found.\n";
    }

    cout << "\nTesting remove for ID 1005:\n";
    if (registry.remove(1005)) cout << "Vehicle 1005 removed from registry.\n";
    else cout << "Vehicle 1005 removal failed.\n";

    cout << "Lookup again for ID 1005:\n";
    found = registry.lookup(1005);
    if (found == NULL) cout << "Vehicle 1005 is no longer in registry.\n";

    TrafficSimulation sim;

    cout << "\nCreating road network A->B, A->C, B->D, C->D, D exit... done.\n";

    // Queue operation demos using intersection A.
    cout << "\nQueue operation demo on Intersection A:\n";
    cout << "Is A empty? " << (sim.getA()->queue.isEmpty() ? "Yes" : "No") << "\n";

    sim.getA()->queue.enqueue(vehicles[0]);
    sim.getA()->queue.enqueue(vehicles[1]);
    sim.getA()->queue.enqueue(vehicles[2]);

    cout << "After enqueue 3 vehicles, A queue: ";
    sim.getA()->queue.printQueue(); cout << "\n";

    Vehicle* peeked = sim.getA()->queue.peek();
    if (peeked != NULL) {
        cout << "Peek A: ID " << peeked->id << "\n";
    }

    Vehicle* dq = sim.getA()->queue.dequeue();
    if (dq != NULL) {
        cout << "Dequeued from A: ID " << dq->id << "\n";
        // put it back for full simulation
        sim.getA()->queue.enqueue(dq);
    }

    cout << "Is A empty now? " << (sim.getA()->queue.isEmpty() ? "Yes" : "No") << "\n";

    // Add remaining vehicles (except removed id 1005) to entry intersection A.
    for (int i = 3; i < VEHICLE_COUNT; i++) {
        if (vehicles[i]->id == 1005) continue;
        sim.getA()->queue.enqueue(vehicles[i]);
    }

    cout << "\nInitial queue at A before FCFS simulation:\n";
    sim.getA()->queue.printQueue(); cout << "\n";

    sim.simulateTraffic(15, FCFS);

    // Add a few new vehicles to show priority behavior clearly.
    Vehicle* p1 = new Vehicle{2001, "EMG001", "ambulance", 200};
    Vehicle* p2 = new Vehicle{2002, "CAR001", "car", 201};
    Vehicle* p3 = new Vehicle{2003, "TRK001", "truck", 202};
    Vehicle* p4 = new Vehicle{2004, "EMG002", "ambulance", 203};

    registry.insert(p1); registry.insert(p2); registry.insert(p3); registry.insert(p4);

    sim.getA()->queue.enqueue(p2);
    sim.getA()->queue.enqueue(p3);
    sim.getA()->queue.enqueue(p1);
    sim.getA()->queue.enqueue(p4);

    cout << "\nAdded extra vehicles for PRIORITY run. Queue A now:\n";
    sim.getA()->queue.printQueue(); cout << "\n";

    sim.simulateTraffic(8, PRIORITY);

    sim.history.printHistory();
    sim.history.saveToFile("history.txt");
    sim.saveQueuesToFile("queues.txt");

    cout << "\nSaved history to history.txt and queues to queues.txt\n";
    cout << "Program finished successfully.\n";

    return 0;
}
