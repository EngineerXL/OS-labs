#ifndef TOPOLOGY_HPP
#define TOPOLOGY_HPP

#include <iostream>
#include <list>

template<class T>
class topology_t {
private:
	using list_type = std::list< std::list<T> >;
	using iterator = typename std::list<T>::iterator;
	using list_iterator = typename list_type::iterator;

	list_type container;
	size_t container_size;
public:
	explicit topology_t() noexcept : container(), container_size(0) {}
	~topology_t() {}

	bool erase(const T & elem) {
		for (list_iterator it1 = container.begin(); it1 != container.end(); ++it1) {
			for (iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
				if (*it2 == elem) {
					if (it1->size() > 1) {
						it1->erase(it2);
					} else {
						container.erase(it1);
					}
					--container_size;
					return true;
				}
			}
		}
		return false;
	}

	long long find(const T & elem) {
		long long ind = 0;
		for (list_iterator it1 = container.begin(); it1 != container.end(); ++it1) {
			for (iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
				if (*it2 == elem) {
					return ind;
				}
			}
			++ind;
		}
		return -1;
	}

	bool insert(const T & parent, const T & elem) {
		for (list_iterator it1 = container.begin(); it1 != container.end(); ++it1) {
			for (iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
				if (*it2 == parent) {
					it1->insert(++it2, elem);
					++container_size;
					return true;
				}
			}
		}
		return false;
	}

	void insert(const T & elem) {
		std::list<T> new_list;
		new_list.push_back(elem);
		++container_size;
		container.push_back(new_list);
	}

	size_t size() {
		return container_size;
	}

	template<class U>
	friend std::ostream & operator << (std::ostream & of, const topology_t<U> & top) {
		for (auto it1 = top.container.begin(); it1 != top.container.end(); ++it1) {
			of << "{";
			for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
				of << *it2 << " ";
			}
			of << "}" << std::endl;
		}
		return of;
	}
};

#endif /* TOPOLOGY_HPP */