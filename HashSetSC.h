#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 7 >
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator;
  using iterator = const_iterator;
// using key_compare = std::less<key_type>;                        // B+-Tree
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing

private:
struct Element {
  key_type key;
  Element* head {nullptr};
  Element* next {nullptr};
};

  Element* table {nullptr};
  size_type table_size{0};
  size_type current_size{0};
  float max_lf {0.7};
  void add(const key_type &key);
  Element *locate(const key_type &key) const;
  size_type h(const key_type &key) const {return hasher {} (key) % table_size; }
  void reserve(size_type n);
  void rehash(size_type n);

public:
  ADS_set()  {rehash(N);}                                                          
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set {} {insert(ilist);}                     // PH1
  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{}{insert(first, last);}   // PH1
  
  ADS_set(const ADS_set &other);

  ~ADS_set() {
    for (size_type index{0}; index < table_size; ++index) {
      Element* elem {table[index].head};
      while (elem != nullptr) {
          Element* next = elem->next;
          delete elem;
          elem = next;
      }
    }
    delete[] table;
  }  

  ADS_set &operator=(const ADS_set &other);
  ADS_set &operator=(std::initializer_list<key_type> ilist);

  size_type size() const {
    return current_size;
  }                                                                   
  
  bool empty() const {
    return current_size == 0;
  }                                                                    

  void insert(std::initializer_list<key_type> ilist)  { insert (ilist.begin(), ilist.end());}
  std::pair<iterator,bool> insert(const key_type &key);
  template<typename InputIt> void insert(InputIt first, InputIt last);

  void clear();
  size_type erase(const key_type &key);

  size_type count(const key_type &key) const { return locate(key) != nullptr; }                          // PH1
  iterator find(const key_type &key) const;

  void swap(ADS_set &other);

  const_iterator begin() const {
      for (size_type index = 0; index < table_size; ++index) {
          if (table[index].head != nullptr) {
              return const_iterator{table[index].head, table, table_size, index};
          }
      }
      return end();
  }


  const_iterator end() const {
    return const_iterator{};
  }
  
  void dump(std::ostream &o = std::cerr) const;
  
  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
    if (lhs.current_size != rhs.current_size) return false;
    for (const auto &k: lhs) if (!rhs.count(k)) return false;
    return true;
  }

  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {return !(lhs == rhs);}
};

template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(const ADS_set &other) {
  rehash(other.table_size);
  for (const auto &k: other) add(k);
}

template <typename Key, size_t N>
void ADS_set<Key,N>::add(const key_type &key) {
  if (current_size > table_size * max_lf) {
    reserve(table_size * 2);
  } 

  size_type index = h(key);

  if (locate(key) == nullptr) { // key ist neu
    Element* elem = new Element();  
    elem->key = key; 
    elem->next = table[index].head;

    table[index].head = elem;
    ++current_size;
  }
} 

template <typename Key, size_t N>
typename ADS_set<Key, N>::Element* ADS_set<Key, N>::locate(const key_type &key) const {
    size_type index = h(key);

    for (Element* elem = table[index].head; elem != nullptr; elem = elem->next) {
      if (key_equal{}(elem->key, key)) return table+index; // return &table[index]; wenn key gefunden
    }

    return nullptr; // falls nichts gefunden
}

template <typename Key, size_t N>
void ADS_set<Key,N>::swap(ADS_set &other) {
  using std::swap;
  swap(table, other.table);
  swap(table_size, other.table_size);
  swap(current_size, other.current_size);
  swap(max_lf, other.max_lf);
}

template <typename Key, size_t N>
typename ADS_set<Key,N>::iterator ADS_set<Key,N>::find(const key_type &key) const {
  size_type index{h(key)};

  if (table[index].head != nullptr) {
    
    for (Element* elem = table[index].head; elem != nullptr; elem = elem->next){
      if (key_equal{}(elem->key, key)) {
        return const_iterator(elem, table, table_size, index); // gefundenes elem als itr returnen
      }
    }
  }
  return end(); // key nicht gefundne
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::erase(const key_type &key) {
  size_type index = h(key);
  Element *old = nullptr;
  Element *current = table[index].head;
  if (!locate(key)) return 0;

  while (current != nullptr && !key_equal{}(current->key, key)) {
    old = current;
    current = current->next;
  }

  if (old != nullptr) { // nicht erstes 
    old->next = current->next;
  } else { // erstes 
    table[index].head = current->next;
  }

  delete current;
  --current_size;
  return 1;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::clear() {
  ADS_set temp;
  swap(temp);
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key, N>::iterator, bool> ADS_set<Key, N>::insert(const key_type &key) {
  if (locate(key) == nullptr) { // noch nicht im set 
    add(key);
    return std::pair(find(key), true);  
  } else { 
    return std::pair(find(key), false);
  }
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last) {
  for (auto it{first}; it != last; ++it) {
    if (!count(*it))  {
      add(*it);
      
    }
  }
}

template <typename Key, size_t N>
ADS_set<Key,N> &ADS_set<Key,N>::operator=(const ADS_set &other) {
  ADS_set temp{other};
  swap(temp);
  return *this;
}

template <typename Key, size_t N>
ADS_set<Key, N> &ADS_set<Key,N>::operator=(std::initializer_list<key_type> ilist) {
  ADS_set temp{ilist};
  swap(temp);
  return *this; 
}

template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream &o) const {
  o << "CURRENT SIZE: " << current_size << " | TABLE SIZE: " << table_size << "\n";
  
     for (size_type index{0}; index < table_size; ++index) {
      o << index << ") ";

      if (table[index].head == nullptr) o << "~ free ~";
    
      for (Element* elem {table[index].head}; elem != nullptr; elem = elem->next) {
        o << elem->key;

        if (elem->next == nullptr) break;

        o << " ~> ";
      }
      o << "\n";
    }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::reserve(size_type n) {
  if (table_size * max_lf >= n) return;

  size_type new_table_size{table_size};
  
  while (new_table_size * max_lf < n) {
    ++(new_table_size *= 2); // ++ falls ts am anfang 0
  }

  rehash(new_table_size);
}

template <typename Key, size_t N>
void ADS_set<Key, N>::rehash(size_type n) {
  size_type new_table_size = std::max(N, std::max(n, size_type(current_size / max_lf)));
  Element* new_table = new Element[new_table_size];
  Element* old_table = table; 
  size_type old_table_size = table_size;
  
  current_size = 0;
  table = new_table;
  table_size = new_table_size; 

  for (size_type index{0}; index < old_table_size; ++index) {
    for (Element* elem = old_table[index].head; elem != nullptr; elem = elem->next) {
      add(elem->key);
      delete elem;
    }
	}
  delete[] old_table;
}

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
  Element *e;
  Element* table;
  size_type table_size;
  size_t index;

  void skip() {
    if (e != nullptr) {
      e = e->next;
    }

    while (index < table_size && e == nullptr) {
      ++index;
      if (index < table_size) {
          e = table[index].head;
      }
    }
  }

public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

  explicit Iterator(Element* e = nullptr, Element* table = nullptr, size_type table_size = 0, size_t index = 0)
    : e{e}, table{table}, table_size{table_size}, index{index} {
     if (e == nullptr && table != nullptr) {
        skip();
    }
  }

  reference operator*() const { return e->key; }
  pointer operator->() const { return &e->key; }
  Iterator &operator++() {  
    skip();
    return *this;
  }

  Iterator operator++(int) { auto rc {*this}; ++*this; return rc; }
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) { return lhs.e == rhs.e; }
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};



template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }
#endif // ADS_SET_H