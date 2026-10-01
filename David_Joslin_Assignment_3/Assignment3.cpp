#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

/**
 * Represents an email with priority attributes.
 * Priority is determined by:
 * 1. Category (Boss > Subordinate > Peer > ImportantPerson > OtherPerson)
 * 2. Date (Newer dates have higher priority)
 */
class Email {
public:
    int category; // Numeric value for priority comparison
    std::string categoryName;
    std::string subject;
    int dateVal; // Converted to YYYYMMDD for easy integer comparison
    std::string dateStr;

    Email(int cat, std::string name, std::string subj, int dateV, std::string dateS)
        : category(cat), categoryName(name), subject(subj), dateVal(dateV), dateStr(dateS) {}

    /**
     * Returns true if this email has higher priority than the other email.
     */
    bool isHigherPriorityThan(const Email& other) const {
        if (category != other.category) {
            return category > other.category;
        }
        return dateVal > other.dateVal;
    }
};

/**
 * MaxHeap implementation from scratch using a vector-based structure.
 * This handles the email prioritization logic.
 */
class MaxHeap {
private:
    std::vector<Email> heap;

    void siftUp(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (heap[idx].isHigherPriorityThan(heap[parent])) {
                std::swap(heap[idx], heap[parent]);
                idx = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(int idx) {
        int n = heap.size();
        while (true) {
            int largest = idx;
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;

            if (left < n && heap[left].isHigherPriorityThan(heap[largest])) {
                largest = left;
            }
            if (right < n && heap[right].isHigherPriorityThan(heap[largest])) {
                largest = right;
            }

            if (largest != idx) {
                std::swap(heap[idx], heap[largest]);
                idx = largest;
            } else {
                break;
            }
        }
    }

public:
    void push(const Email& e) {
        heap.push_back(e);
        siftUp(heap.size() - 1);
    }

    void pop() {
        if (heap.empty()) return;
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            siftDown(0);
        }
    }

    Email* peek() {
        if (heap.empty()) return nullptr;
        return &heap[0];
    }

    size_t size() const {
        return heap.size();
    }
};

/**
 * Manages the CEO's inbox and processes commands.
 */
class EmailManager {
private:
    MaxHeap inbox;
    bool firstOutput = true;

    void printBlankLine() {
        if (!firstOutput) {
            std::cout << std::endl;
        }
    }

public:
    void processCommand(const std::string& line) {
        if (line.empty()) return;

        if (line.substr(0, 5) == "EMAIL") {
            // Format: EMAIL Category,Subject,Date
            std::string content = line.substr(6);
            std::stringstream ss(content);
            std::string segment;
            std::vector<std::string> parts;

            while (std::getline(ss, segment, ',')) {
                parts.push_back(segment);
            }

            if (parts.size() >= 3) {
                std::string catStr = parts[0];
                std::string subj = parts[1];
                std::string dateStr = parts[2];

                int catVal = 0;
                std::string catName = "";

                // Map categories to priority values
                if (catStr == "Boss") { catVal = 4; catName = "Boss"; }
                else if (catStr == "Subordinate") { catVal = 3; catName = "Subordinate"; }
                else if (catStr == "Peer") { catVal = 2; catName = "Peer"; }
                else if (catStr == "ImportantPerson") { catVal = 1; catName = "ImportantPerson"; }
                else if (catStr == "OtherPerson") { catVal = 0; catName = "OtherPerson"; }

                // Parse date MM-DD-YYYY to YYYYMMDD integer
                int m, d, y;
                sscanf(dateStr.c_str(), "%d-%d-%d", &m, &d, &y);
                int dateVal = y * 10000 + m * 100 + d;

                inbox.push(Email(catVal, catName, subj, dateVal, dateStr));
            }
        } else if (line == "NEXT") {
            Email* e = inbox.peek();
            if (e) {
                printBlankLine();
                std::cout << "Next email:" << std::endl;
                std::cout << "\tSender: " << e->categoryName << std::endl;
                std::cout << "\tSubject: " << e->subject << std::endl;
                std::cout << "\tDate: " << e->dateStr << std::endl;
                firstOutput = false;
            }
        } else if (line == "READ") {
            inbox.pop();
        } else if (line == "COUNT") {
            printBlankLine();
            std::cout << "There are " << inbox.size() << " emails to read." << std::endl;
            firstOutput = false;
        }
    }
};

int main(int argc, char* argv[]) {
    std::istream* input;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            // Fallback to cin if file fails to open
            input = &std::cin;
        } else {
            input = &file;
        }
    } else {
        input = &std::cin;
    }

    EmailManager manager;
    std::string line;
    while (std::getline(*input, line)) {
        manager.processCommand(line);
    }

    return 0;
}

