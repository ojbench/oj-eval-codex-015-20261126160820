#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>

namespace {
constexpr int capacity = 64;
struct Key {
    char name[65];
    int value;
};
int compare(const Key &a, const Key &b) {
    int c = std::strcmp(a.name, b.name);
    return c ? c : (a.value > b.value) - (a.value < b.value);
}
struct Page {
    int count, leaf, next;
    Key keys[capacity + 1];
    int children[capacity + 2];
};
struct Header { int root, pages; };

class Database {
    int fd;
    Header header;
    void read(int id, Page &page) {
        if (pread(fd, &page, sizeof(page), sizeof(Header) + off_t(id) * sizeof(Page))
            != ssize_t(sizeof(page))) std::exit(1);
    }
    void write(int id, const Page &page) {
        if (pwrite(fd, &page, sizeof(page), sizeof(Header) + off_t(id) * sizeof(Page))
            != ssize_t(sizeof(page))) std::exit(1);
    }
    void save_header() {
        if (pwrite(fd, &header, sizeof(header), 0) != ssize_t(sizeof(header))) std::exit(1);
    }
    static int lower(const Page &p, const Key &key) {
        int l = 0, r = p.count;
        while (l < r) {
            int m = (l + r) / 2;
            if (compare(p.keys[m], key) < 0) l = m + 1;
            else r = m;
        }
        return l;
    }
    static int branch(const Page &p, const Key &key) {
        int pos = lower(p, key);
        if (pos < p.count && compare(p.keys[pos], key) == 0) ++pos;
        return pos;
    }
    // A split returns the new right page and its separator to the parent.
    int insert(int id, const Key &key, Key &separator) {
        Page p;
        read(id, p);
        int pos;
        if (p.leaf) {
            pos = lower(p, key);
            if (pos < p.count && compare(p.keys[pos], key) == 0) return -1;
            std::memmove(p.keys + pos + 1, p.keys + pos, (p.count - pos) * sizeof(Key));
            p.keys[pos] = key;
        } else {
            pos = branch(p, key);
            Key promoted;
            int right = insert(p.children[pos], key, promoted);
            if (right == -1) return -1;
            std::memmove(p.keys + pos + 1, p.keys + pos, (p.count - pos) * sizeof(Key));
            std::memmove(p.children + pos + 2, p.children + pos + 1,
                         (p.count - pos) * sizeof(int));
            p.keys[pos] = promoted;
            p.children[pos + 1] = right;
        }
        if (++p.count <= capacity) {
            write(id, p);
            return -1;
        }
        Page right{};
        right.leaf = p.leaf;
        int mid = p.count / 2;
        separator = p.keys[mid];
        int start = mid + (p.leaf ? 0 : 1);
        right.count = p.count - start;
        std::memcpy(right.keys, p.keys + start, right.count * sizeof(Key));
        if (!p.leaf)
            std::memcpy(right.children, p.children + start, (right.count + 1) * sizeof(int));
        int new_id = header.pages++;
        right.next = p.next;
        if (p.leaf) p.next = new_id;
        p.count = mid;
        write(new_id, right);
        write(id, p);
        return new_id;
    }
public:
    Database() {
        fd = open("storage.db", O_RDWR | O_CREAT, 0600);
        if (fd < 0) std::exit(1);
        ssize_t bytes = pread(fd, &header, sizeof(header), 0);
        if (bytes == 0) {
            header = {0, 1};
            Page root{};
            root.leaf = 1;
            root.next = -1;
            write(0, root);
            save_header();
        } else if (bytes != ssize_t(sizeof(header))) std::exit(1);
    }
    ~Database() { close(fd); }
    void insert(const Key &key) {
        int old_pages = header.pages;
        Key separator;
        int right = insert(header.root, key, separator);
        if (right != -1) {
            Page root{};
            root.count = 1;
            root.keys[0] = separator;
            root.children[0] = header.root;
            root.children[1] = right;
            header.root = header.pages++;
            write(header.root, root);
        }
        if (old_pages != header.pages) save_header();
    }
    void erase(const Key &key) {
        Page p;
        int id = header.root;
        read(id, p);
        while (!p.leaf) {
            id = p.children[branch(p, key)];
            read(id, p);
        }
        int pos = lower(p, key);
        if (pos == p.count || compare(p.keys[pos], key)) return;
        std::memmove(p.keys + pos, p.keys + pos + 1, (p.count - pos - 1) * sizeof(Key));
        --p.count;
        // Separators remain valid lower bounds even when a leaf becomes empty.
        write(id, p);
    }
    void find(const Key &key) {
        Page p;
        read(header.root, p);
        while (!p.leaf) {
            int id = p.children[branch(p, key)];
            read(id, p);
        }
        int pos = lower(p, key);
        bool found = false;
        for (;;) {
            for (; pos < p.count; ++pos) {
                if (std::strcmp(p.keys[pos].name, key.name)) {
                    if (!found) std::printf("null");
                    std::putchar('\n');
                    return;
                }
                if (found) std::putchar(' ');
                std::printf("%d", p.keys[pos].value);
                found = true;
            }
            if (p.next == -1) break;
            int next = p.next;
            read(next, p);
            pos = 0;
        }
        if (!found) std::printf("null");
        std::putchar('\n');
    }
};
}

int main() {
    Database database;
    int n;
    if (std::scanf("%d", &n) != 1) return 0;
    while (n--) {
        char command[8];
        Key key{};
        if (std::scanf("%7s%64s", command, key.name) != 2) return 1;
        if (command[0] == 'f') database.find(key);
        else {
            if (std::scanf("%d", &key.value) != 1) return 1;
            if (command[0] == 'i') database.insert(key);
            else database.erase(key);
        }
    }
}
