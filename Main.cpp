#include <iostream>
#include <iomanip>
#include <cinttypes>
#include <experimental/filesystem>
#include <algorithm>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

#include "Node.cpp"

namespace fs = std::experimental::filesystem;

std::atomic<bool> exit_flag;    // ������, ����������� ���������� ��������
std::string detected_path;      // ��������� ����

namespace thread_func {
    void findFileByName(Node* root, std::string name) {
        // ���� ����� ����� �������� ������ ������ �������� - �������, 
        // ������������ ������������� ���� � ������� ������� ��������, ���� ���� ����������� �������� ������
        if (strcmp(name.c_str(), root->path.filename().string().c_str()) == 0 && !root->is_dir) {
            if (exit_flag.load())
                return;
            detected_path = root->path.string();
            exit_flag.store(true);
        }



        // ���� ����������, �� ������� ������ ��� ���� �� �������� ���������, ������ ����� �������� �� ���������
        if (root->is_dir) {
            for (Node* object : root->children) {
                if (exit_flag.load())
                    return;
                std::thread([object, name]() { thread_func::findFileByName(object, name); }).detach();
            }
        }
    }
}

// �����������(���������) �������, ����������� ������
void findFileByName(Node* root, const std::string& name) {

    if (strcmp(name.c_str(), root->path.filename().string().c_str()) == 0 && !root->is_dir) {
        if (exit_flag.load())
            return;
        exit_flag.store(true);
        detected_path = root->path.string();
        return;
    }


    if (root->is_dir)
        for (Node* object : root->children) {
            if (exit_flag.load())
                return;
            std::thread([object, name]() { thread_func::findFileByName(object, name); }).detach();
        }
}

// ����� ����������� ������
void printDirectoryTree(Node* object) {
    if (!object)
        return;

    if (object->is_dir) {
        std::cout << std::setw(object->depth * 3) << "" << object->name << " - DIR\n";
        for (const auto& child : object->children) {
            printDirectoryTree(child);
        }
        return;
    }

    if (object->is_other) {
        std::cout << std::setw(object->depth * 3) << "" << object->name << " - OTHER\n";
        return;
    }

    std::cout << std::setw(object->depth * 3) << "" << object->name << " - FILE(" << object->size << " bytes)\n";
}

// ���������� ��������� ������ � ��������������� ����� ����� std::filesystem
void getDirectoryTree(Node* root, const fs::path& pathToScan) {
    for (const auto& entry : fs::directory_iterator(pathToScan)) {
        auto filename = entry.path().filename().string();

        root->name = pathToScan.filename().string();
        root->path = pathToScan;
        root->is_dir = true;

        Node* new_node = new Node();
        new_node->name = filename;
        new_node->depth = root->depth + 1;
        new_node->path = entry.path();

        root->addChild(new_node);

        if (entry.is_directory()) {
            new_node->is_dir = true;
            getDirectoryTree(new_node, entry);
        }
        else if (entry.is_regular_file()) {
            new_node->size = entry.file_size();
        }
        else {
            new_node->is_other = true;
        }
    }
}

// ����������� ������� ������
void clearTree(Node* root) {
    if (!root->is_dir) {
        delete root;
        return;
    }

    for (auto& ptr : root->children)
        clearTree(ptr);

    delete root;
}

int main() {
    // �������������� ������ �������� ��������� ���� � �������
    // �������� ������, ���� ���� �� ����������
    Node* baseRoot = new Node();

    // ���������� ������
    getDirectoryTree(baseRoot, fs::path("/home/kondrativvo/DocSup/TestDir"));

    // ����� ������
    printDirectoryTree(baseRoot);

    // ������������� ��������� �������� ��� ������ � ��������� ����
    exit_flag.store(false);
    detected_path = "";

    // �������� ����� �� ��������
    findFileByName(baseRoot, "1.txt");

    // ���� �� ����������� ����, �������� ������� �����
    while (!exit_flag.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // ������� ��������� ����
    std::cout << "Path to file: " << detected_path << std::endl;

    // ������� ������
    clearTree(baseRoot);
    return 0;
}