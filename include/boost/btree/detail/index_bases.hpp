//  boost/btree/detail/index_bases.hpp  ------------------------------------------------//

//  Copyright Beman Dawes 2000, 2006, 2010, 2013

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/btree for documentation.

#ifndef BOOST_BTREE_INDEX_BASES_HPP
#define BOOST_BTREE_INDEX_BASES_HPP

#include <boost/btree/helpers.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/btree/mmff.hpp>
#include <boost/btree/btree_set.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/assert.hpp>

/*

  TODO:

  *  Does insert/emplace unique of an existing entry return the correct .first iterator?

  *  verify dereferencing the end iterator assert fires correctly.

  *  sets, maps, missing close(). Might need an argument that says what to close. The index?
     the flat file? Both?

  *  Shouldn't there be a file_reserve setter?
  *  file_reserve setter should round up to memory map page size boundary.
  *  Open should call the file_reserve setter.

  *  Abstract away the difference between index_set_base and index_multiset_base,
     index_map_base and index_multimap_base?

*/

namespace boost
{
namespace btree
{
namespace detail
{
  template <class T, class Pos, class Compare>
    class indirect_compare;
}

//--------------------------------------------------------------------------------------//
//                                class index_set_base                                  //
//--------------------------------------------------------------------------------------//

template <class Key, class BtreeTraits, class Compare>
class index_set_base
{
public:
  typedef Key                                    key_type;
  typedef Key                                    value_type;
  typedef Key const                              iterator_value_type;
  typedef Key                                    mapped_type;
  typedef BtreeTraits                            btree_traits;
  typedef Compare                                compare_type;
  typedef compare_type                           value_compare;
  typedef typename BtreeTraits::node_id_type     node_id_type;
  typedef typename BtreeTraits::node_size_type   node_size_type;
  typedef typename BtreeTraits::node_level_type  node_level_type;
  typedef typename BtreeTraits::index_position_type  index_position_type;

  //  reference is the type returned from dereferencing an iterator, so may be a Key&
  //  or a Key value. The former occurs when the actual data stored in the flat file is
  //  of type Key. The latter occurs when the Key object returned is a proxy pointing to
  //  the actual data stored in the flat file; an example is string_view.
  typedef 
    typename index_reference<value_type>::type  reference;

  typedef detail::indirect_compare<key_type,
    index_position_type, compare_type>           index_compare_type;

  //  the following is the only difference between index_set_base and index_multiset_base
  typedef typename
    btree::btree_set<index_position_type,
      btree_traits, index_compare_type>          index_type;

  // TODO: why aren't these static?
  const Key&          key(const value_type& v) const   // really handy, so expose
    {return v;}
  const mapped_type&  mapped(const value_type& v) const
    {return v;}

protected:
  static reference dereference(const char* p)
  {
    return index_deserialize<value_type>(&p);
  }
};

//--------------------------------------------------------------------------------------//
//                             class index_multiset_base                                //
//--------------------------------------------------------------------------------------//

template <class Key, class BtreeTraits, class Compare>
class index_multiset_base
{
public:
  typedef Key                                    key_type;
  typedef Key                                    value_type;
  typedef Key const                              iterator_value_type;
  typedef Key                                    mapped_type;
  typedef BtreeTraits                            btree_traits;
  typedef Compare                                compare_type;
  typedef compare_type                           value_compare;

  typedef typename BtreeTraits::node_id_type     node_id_type;
  typedef typename BtreeTraits::node_size_type   node_size_type;
  typedef typename BtreeTraits::node_level_type  node_level_type;
  typedef typename BtreeTraits::index_position_type  index_position_type;

  //  Comment for index_set_base::reference also applies here
  typedef
    typename index_reference<value_type>::type   reference;

  typedef detail::indirect_compare<key_type,
    index_position_type, compare_type>           index_compare_type;

  //  the following is the only difference between index_set_base and index_multiset_base
  typedef typename
    btree::btree_multiset<index_position_type,
      btree_traits, index_compare_type>          index_type;

  // TODO: why aren't these static?
  const Key&          key(const value_type& v) const   // really handy, so expose
    {return v;}
  const mapped_type&  mapped(const value_type& v) const
    {return v;}

protected:
  static reference dereference(const char* p)
  {
    return index_deserialize<value_type>(&p);
  }
};

//--------------------------------------------------------------------------------------//
//                               class index_map_base                                   //
//--------------------------------------------------------------------------------------//

template <class Key, class T, class BtreeTraits, class Compare>
class index_map_base
{
public:
  typedef Key                                    key_type;
  typedef T                                      mapped_type;
  typedef std::pair<
    const typename index_reference<Key>::type,
    typename index_reference<T>::type>           value_type;
  typedef value_type                             iterator_value_type;
  typedef Compare                                compare_type;
  typedef BtreeTraits                            btree_traits;
  typedef typename BtreeTraits::node_id_type     node_id_type;
  typedef typename BtreeTraits::node_size_type   node_size_type;
  typedef typename BtreeTraits::node_level_type  node_level_type;
  typedef typename BtreeTraits::index_position_type  index_position_type;

  //  reference is the type returned from dereferencing an iterator; it is currently
  //  always of type value_type, NOT value_type&. This is because the value_type object
  //  returned is a proxy whose first member will be of type Key& or Key, and whose
  //  second member will be of type T& or T. Whether the member type is a reference or
  //  value type is determined by index_reference<Key>::type and index_reference<T>::type
  //  respectively. 
  typedef value_type                             reference;                

  typedef detail::indirect_compare<key_type,
    index_position_type, compare_type>           index_compare_type;

  //  the following is the only difference between index_map_base and index_multimap_base
  typedef typename
    btree::btree_set<index_position_type,
      btree_traits, index_compare_type>          index_type;

  const Key&  key(const value_type& v) const  // really handy, so expose
    {return v.first;}
  const T&    mapped(const value_type& v) const
    {return v.second;}

  class value_compare
  {
  public:
    value_compare() {}
    value_compare(compare_type comp) : m_comp(comp) {}
    bool operator()(const value_type& x, const value_type& y) const
      { return m_comp(x.first, y.first); }
    template <class K>
    bool operator()(const value_type& x, const K& y) const
      { return m_comp(x.first, y); }
    template <class K>
    bool operator()(const K& x, const value_type& y) const
      { return m_comp(x, y.first); }
  private:
    compare_type    m_comp;
  };

protected:
  static reference dereference(const char* p)
  {
    typename index_reference<key_type>::type first
      = index_deserialize<key_type>(&p);
    typename index_reference<mapped_type>::type second
      = index_deserialize<mapped_type>(&p);
    return reference(first, second);
  }
};

//--------------------------------------------------------------------------------------//
//                            class index_multimap_base                                 //
//--------------------------------------------------------------------------------------//

template <class Key, class T, class BtreeTraits, class Compare>
class index_multimap_base
{
public:
  typedef Key                                    key_type;
  typedef T                                      mapped_type;
  typedef std::pair<
    const typename index_reference<Key>::type,
    typename index_reference<T>::type>           value_type;
  typedef value_type                             iterator_value_type;
  typedef Compare                                compare_type;
  typedef BtreeTraits                            btree_traits;
  typedef typename BtreeTraits::node_id_type     node_id_type;
  typedef typename BtreeTraits::node_size_type   node_size_type;
  typedef typename BtreeTraits::node_level_type  node_level_type;
  typedef typename BtreeTraits::index_position_type  index_position_type;

  //  Comment for index_map_base::reference also applies here
  typedef value_type                             reference;                

  typedef detail::indirect_compare<key_type,
    index_position_type, compare_type>           index_compare_type;

  //  the following is the only difference between index_map_base and index_multimap_base
  typedef typename
    btree::btree_multiset<index_position_type,
      btree_traits, index_compare_type>          index_type;

  const Key&  key(const value_type& v) const  // really handy, so expose
    {return v.first;}
  const T&    mapped(const value_type& v) const
    {return v.second;}

  class value_compare
  {
  public:
    value_compare() {}
    value_compare(compare_type comp) : m_comp(comp) {}
    bool operator()(const value_type& x, const value_type& y) const
      { return m_comp(x.first, y.first); }
    template <class K>
    bool operator()(const value_type& x, const K& y) const
      { return m_comp(x.first, y); }
    template <class K>
    bool operator()(const K& x, const value_type& y) const
      { return m_comp(x, y.first); }
  private:
    compare_type    m_comp;
  };

protected:
  static reference dereference(const char* p)
  {
    typename index_reference<key_type>::type first
      = index_deserialize<key_type>(&p);
    typename index_reference<mapped_type>::type second
      = index_deserialize<mapped_type>(&p);
    return reference(first, second);
  }
};

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                                 class index_base                                     //
//                                                                                      //
//--------------------------------------------------------------------------------------//


template <class Base>  // index_[multi]map_base or index_[multi]set_base
class index_base : public Base, private noncopyable
{
public:
  template <class T, class Reference>
    class iterator_type;

//--------------------------------------------------------------------------------------//
//                                public interface                                      //
//--------------------------------------------------------------------------------------//

public:
  // types:
  typedef typename Base::key_type               key_type;
  typedef typename Base::value_type             value_type;
  typedef typename Base::mapped_type            mapped_type;
  typedef typename Base::compare_type           compare_type;
  typedef typename Base::compare_type           key_compare;
  typedef typename Base::value_compare          value_compare; 
  typedef boost::uint64_t                       size_type;

  typedef typename Base::reference              reference;
  typedef const reference                       const_reference;
  typedef value_type*                           pointer;
  typedef const value_type*                     const_pointer;

  // for sets, these are the same type; for maps they are different types
  typedef iterator_type<typename 
    Base::iterator_value_type, reference>       iterator;
  typedef iterator_type<value_type const,
    reference>                                  const_iterator;

  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::pair<const_iterator, const_iterator>
                                                const_iterator_range;

  typedef typename Base::btree_traits           btree_traits;
  typedef typename Base::node_id_type           node_id_type;
  typedef typename Base::node_size_type         node_size_type;
  typedef typename Base::node_level_type        node_level_type;

  typedef boost::filesystem::path               path_type;

  typedef boost::btree::extendible_mapped_file  file_type;
  typedef boost::shared_ptr<file_type>          file_ptr_type;
  typedef file_type::size_type                  file_size_type;
  typedef file_type::position_type              file_position;

//  typedef typename Base::index_traits           index_traits;
  typedef typename Base::index_position_type      index_position_type;  // i.e. position in flat file

  typedef typename Base::index_compare_type     index_compare_type;

  typedef typename Base::index_type             index_type;

  typedef typename index_type::size_type        index_size_type;


protected:
  index_type         m_index_btree;
  file_ptr_type      m_file;       // shared_ptr to flat file; shared with other indexes
  key_compare        m_comp;
  value_compare      m_value_comp;
public:
  index_base() {} 

  //  opens

  void open(const path_type& index_pth,
            const path_type& file_pth,
            flags::bitmask flgs = flags::read_only,
            uint64_t sig = -1,  // for existing files, must match creation signature
            const compare_type& comp = compare_type(),
            std::size_t node_sz = default_node_size)  // node_sz ignored if existing file
  {
    BOOST_ASSERT(!m_index_btree.is_open());
    BOOST_ASSERT(!m_file.get());
    m_file.reset(new file_type);
    m_file->open(file_pth, flgs, reserve_default(flgs));
    open(index_pth, m_file, flgs, sig, comp, node_sz);
  }

  void open(const path_type& index_pth,
            file_ptr_type flat_file,            
            flags::bitmask flgs = flags::read_only,
            uint64_t sig = -1,  // for existing files, must match creation signature
            const compare_type& comp = compare_type(),
            std::size_t node_sz = default_node_size)  // node_sz ignored if existing file
  {
    BOOST_ASSERT(!m_index_btree.is_open());
    BOOST_ASSERT(flat_file->is_open());
    m_file = flat_file;
    m_comp = comp;
    m_index_btree.open(index_pth, flgs, sig,
      index_compare_type(m_comp, m_file.get()), node_sz);
  }

  //  modifiers
  iterator erase(const_iterator itr)
  {
    typename index_type::const_iterator result(m_index_btree.erase(itr.m_index_iterator));
    return result == m_index_btree.cend()
      ? end()
      : iterator(result, m_file);
  }
  size_type erase(const key_type& k)
  {
    size_type count = 0;
    const_iterator itr = lower_bound(k);
    
    while (itr != end() && !m_index_btree.key_comp()(k, *itr.m_index_iterator))
    {
      ++count;
      itr = erase(itr);
    }
    return count;
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    return iterator(m_index_btree.erase(first.m_index_iterator, last.m_index_iterator),
      m_file);
  }
  void              clear()                 {m_index_btree.clear();}

  // iterators

  const_iterator begin() const  {return const_iterator(m_index_btree.begin(), file());}
  const_iterator end() const    {return const_iterator(m_index_btree.end(), file());}
  const_reverse_iterator
                     rbegin() const         { return reverse_iterator(cend()); }     
  const_reverse_iterator
                     rend() const           { return reverse_iterator(cbegin()); }
  const_iterator     cbegin() const         { return begin(); }
  const_iterator     cend() const           { return end(); }
  const_reverse_iterator
                     crbegin() const        { return reverse_iterator(cend()); }     
  const_reverse_iterator                    
                     crend() const          { return reverse_iterator(cbegin()); }

  // observers
  bool              is_open() const         {BOOST_ASSERT(!m_index_btree.is_open()
                                               || m_file->is_open());
                                             return m_index_btree.is_open();}
  path_type         path() const            {return m_index_btree.path();}
  flags::bitmask    flags() const           {return m_index_btree.flags();}
  key_compare       key_comp() const        {return m_comp;}
  value_compare     value_comp() const      {return m_value_comp;}
  bool              ok_to_pack() const      {return m_index_btree.ok_to_pack();}
  const buffer_manager&
                    manager() const         {return m_index_btree.manager();} 
  const header_page& header() const         {return m_index_btree.header();}
                                            
  // capacity                               
  bool              empty() const           {return m_index_btree.empty();}
  size_type         size() const            {return m_index_btree.size();}
  size_type         max_size() const        {return m_index_btree.max_size();}
  std::size_t       node_size() const       {return m_index_btree.node_size();}
  std::size_t       max_cache_size() const  {return m_index_btree.max_cache_size();}

  // tuning
  void              max_cache_size(std::size_t m)  // -1 indicates unlimited
                                            {m_index_btree.max_cache_size(m);}
  void              max_cache_megabytes(std::size_t mb)
                                            {m_index_btree.max_cache_megabytes(mb);}
  // file operations
  // TODO: what about flat file flush/close?
  void              flush()                 {m_index_btree.flush();}
  void              close()                 {m_index_btree.close();}

  // flat file
  file_ptr_type     file() const            {return m_file;}
  path_type         file_path() const       {BOOST_ASSERT(m_file);
                                             return m_file->path();}
  file_size_type    file_size() const       {BOOST_ASSERT(m_file);
                                             return m_file->file_size();}
  file_size_type    file_reserve() const    {BOOST_ASSERT(m_file);
                                             return m_file->reserve();}

  // operations
  file_position     position(iterator itr) const; // Returns: The offset in the flat file
                                                  // of the element pointed to by itr
  template <class K>
  const_iterator    find(const K& k) const          {return const_iterator(
                                                          m_index_btree.find(k), m_file);}
  const_iterator    find(const key_type& k) const   {return find<key_type>(k);}
 
  template <class K>
  size_type         count(const K& k) const         {return m_index_btree.count(k);}
  size_type         count(const key_type& k) const  {return count<key_type>(k);}

  template <class K>
  const_iterator    lower_bound(const K& k) const   {return const_iterator(
                                                   m_index_btree.lower_bound(k), m_file);}
  const_iterator    lower_bound(const key_type& k) const{return lower_bound<key_type>(k);}

  template <class K>
  const_iterator    upper_bound(const K& k) const   {return const_iterator(
                                                   m_index_btree.upper_bound(k), m_file);}
  const_iterator    upper_bound(const key_type& k) const{return upper_bound<key_type>(k);}

  template <class K>
  const_iterator_range
                    equal_range(const K& k) const
                                  {return std::make_pair(lower_bound(k), upper_bound(k));}
  const_iterator_range
                    equal_range(const key_type& k) const
    {return std::make_pair(lower_bound<key_type>(k), upper_bound<key_type>(k));}

  //------------------------------------------------------------------------------------//
  //                                  iterator_type                                     //
  //------------------------------------------------------------------------------------//
 
  template <class T, class Reference>
  class iterator_type
    : public boost::iterator_facade<iterator_type<T, Reference>, T,
        bidirectional_traversal_tag, Reference>
  {
  public:
    typedef typename index_type::iterator   index_iterator_type;
    typedef typename index_base::file_type  file_type;

    iterator_type() {}  // constructs the end iterator

    iterator_type(index_iterator_type itr, const file_ptr_type& file)
      : m_index_iterator(itr), m_file(file.get()) {}

  private:
    friend class boost::iterator_core_access;
    friend class index_base;

    index_iterator_type   m_index_iterator;
// use original type "boost::btree::extendible_mapped_file"
// instead of typedef type "file_type" to avoid trouble
// with the i686-apple-darwin11-llvm-g++-4.2 (gcc)4.2.1 compiler
//    file_type*            m_file;  // 0 for end iterator
    boost::btree::extendible_mapped_file* m_file;  // 0 for end iterator

    Reference dereference() const
    { 
      BOOST_ASSERT_MSG(m_file, "btree index attempt to dereference end iterator");
//      std::cout << "**** " << *m_index_iterator << std::endl;
      return Base::dereference(m_file->const_data<char>()
        + static_cast<std::size_t>(*m_index_iterator));
    }
 
    bool equal(const iterator_type& rhs) const
    {
      BOOST_ASSERT(m_file == rhs.m_file);
      return m_index_iterator == rhs.m_index_iterator;
    }

    void increment() { ++m_index_iterator; }
    void decrement() { --m_index_iterator; }
  };

};  // class index_base

namespace detail
{
  // TODO: Pos needs to be a distinct type so no ambiguity arises if Key and
  // index_position_type happen to be the same type. Or else use the heterogenous type
  // approach now used in the std library

  template <class Key, class Pos, class Compare>
  class indirect_compare
  {
    Compare                        m_comp;
    extendible_mapped_file*        m_file;

  public:

    indirect_compare(){}
    indirect_compare(Compare comp, extendible_mapped_file* flat_file)
      : m_comp(comp), m_file(flat_file) {}

    bool operator()(const Key& lhs, Pos rhs) const
    {
      const char* rhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(rhs);
      return m_comp(lhs, index_deserialize<Key>(&rhsp));
    }
    template <class K>
    bool operator()(const K& lhs, Pos rhs) const
    {
      const char* rhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(rhs);
      return m_comp(lhs, index_deserialize<Key>(&rhsp));
    }
    bool operator()(Pos lhs, const Key& rhs) const
    {
      const char* lhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(lhs);
      return m_comp(index_deserialize<Key>(&lhsp), rhs);
    }
    template <class K>
    bool operator()(Pos lhs, const K& rhs) const
    {
      const char* lhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(lhs);
      return m_comp(index_deserialize<Key>(&lhsp), rhs);
    }
    bool operator()(Pos lhs, Pos rhs) const
    {
      const char* lhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(lhs);
      const char* rhsp = m_file->const_data<char>()
          + static_cast<std::size_t>(rhs);
      return m_comp(index_deserialize<Key>(&lhsp), index_deserialize<Key>(&rhsp));
    }

  };

}  // namespace detail

//--------------------------------------------------------------------------------------//
//                              non-member operator <<                                  //
//--------------------------------------------------------------------------------------//

template <class Base>   
std::ostream& operator<<(std::ostream& os,
  const index_base<Base>& bt)
{
  BOOST_ASSERT(bt.is_open());
  os << "  element count ------------: " << bt.header().element_count() << "\n" 
     << "  node size ----------------: " << bt.header().node_size() << "\n"
     << "  levels in tree -----------: " << bt.header().root_level()+1 << "\n"
     << "  node count, inc free list-: " << bt.header().node_count() << "\n"
     << "  leaf node count ----------: " << bt.header().leaf_node_count() << "\n"
     << "  branch node count --------: " << bt.header().branch_node_count() << "\n"
     << "  node count, without free -: " << bt.header().leaf_node_count()
                                            + bt.header().branch_node_count() << "\n"
     << "  root node id -------------: " << bt.header().root_node_id() << "\n"
     << "  free node list head id ---: " << bt.header().free_node_list_head_id() << "\n"
     << "  User supplied string -----: \"" << bt.header().user_c_str() << "\"\n"
     << "  OK to pack ---------------: " << bt.ok_to_pack() << "\n"
  ;
  return os;
}


}  // namespace btree
}  // namespace boost

#endif // BOOST_BTREE_INDEX_BASES_HPP

