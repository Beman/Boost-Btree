//  boost/btree/map.hpp  ---------------------------------------------------------------//

//  Copyright Beman Dawes 2000, 2006, 2010

//  Distributed under the Boost Software License, Version 1.0.
//  http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/btree for documentation.

#ifndef BOOST_BTREE_MAP_HPP
#define BOOST_BTREE_MAP_HPP

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/btree/header.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/btree/detail/vcommon.hpp>  // interface common to vbtree_map and btree_set
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pointer.hpp>

namespace boost
{
  namespace btree
  {

    //  endian traits are the default since they don't require page_id_type alignment

//--------------------------------------------------------------------------------------//
//                                 class vbtree_map                                     //
//--------------------------------------------------------------------------------------//

    template <class Key, class T, class Traits = default_endian_traits,
              class Comp = btree::less<Key> >
    class vbtree_map
      : public vbtree_base<Key, vbtree_map_base<Key,T,Comp>, Traits, Comp>
    {
    public:

      BOOST_STATIC_ASSERT_MSG( !boost::is_pointer<Key>::value, "Key must not be a pointer type");
      BOOST_STATIC_ASSERT_MSG( !boost::is_pointer<T>::value, "T must not be a pointer type");

      // <Key,T,Comp> is required by GCC but not by VC++
      explicit vbtree_map(const Comp& comp = Comp())
        : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(comp) {}

      explicit vbtree_map(const boost::filesystem::path& p,
          flags::bitmask flgs = flags::read_only,
          std::size_t pg_sz = default_page_size,  // ignored if existing file
          const Comp& comp = Comp())
        : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(p,
            flags::user(flgs) | flags::unique, pg_sz, comp) {}

      template <class InputIterator>
      vbtree_map(InputIterator begin, InputIterator end,
        const boost::filesystem::path& p,
        flags::bitmask flgs = flags::read_only,
        std::size_t pg_sz = default_page_size,  // ignored if existing file
        const Comp& comp = Comp())
      : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(p,
          flags::user(flgs) | flags::unique, pg_sz, comp)
      {
        for (; begin != end; ++begin)
          vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_unique(
            begin->key(), begin->mapped_value());
      }
 
      void open(const boost::filesystem::path& p,
        flags::bitmask flgs = flags::read_only,
        std::size_t pg_sz = default_page_size) // pg_sz ignored if existing file
      {
        vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_open(p,
          flags::user(flgs) | flags::unique, pg_sz);
      }

      // typename vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>:: is required by GCC but not VC++
      std::pair<typename vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::const_iterator, bool>
      insert(const Key& key, const T& mapped_value)
      {
        return vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_unique(
          key, mapped_value);
      }

      template <class InputIterator>
      void insert(InputIterator begin, InputIterator end)
      { 
        for (; begin != end; ++begin) 
          vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_unique(
            begin->key(), begin->mapped_value());
      }
    };

//--------------------------------------------------------------------------------------//
//                               class vbtree_multimap                                  //
//--------------------------------------------------------------------------------------//

    template <class Key, class T, class Traits = default_endian_traits,
              class Comp = btree::less<Key> >
    class vbtree_multimap
      : public vbtree_base<Key, vbtree_map_base<Key,T,Comp>, Traits, Comp>
    {
    public:

      BOOST_STATIC_ASSERT_MSG( !boost::is_pointer<Key>::value, "Key must not be a pointer type");
      BOOST_STATIC_ASSERT_MSG( !boost::is_pointer<T>::value, "T must not be a pointer type");

      // <Key,T,Comp> is required by GCC but not by VC++
      explicit vbtree_multimap(const Comp& comp = Comp())
        : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(comp) {}

      explicit vbtree_multimap(const boost::filesystem::path& p,
          flags::bitmask flgs = flags::read_only,
          std::size_t pg_sz = default_page_size,  // ignored if existing file
          const Comp& comp = Comp())
        : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(p,
            flags::user(flgs), pg_sz, comp) {}

      template <class InputIterator>
      vbtree_multimap(InputIterator begin, InputIterator end,
          const boost::filesystem::path& p,
          flags::bitmask flgs = flags::read_only,
          std::size_t pg_sz = default_page_size,  // ignored if existing file
          const Comp& comp = Comp())
        : vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>(p,
            flags::user(flgs), pg_sz, comp)
      {
        for (; begin != end; ++begin)
          vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_non_unique(
            begin->key(), begin->mapped_value());
      }

      void open(const boost::filesystem::path& p,
      flags::bitmask flgs = flags::read_only,
      std::size_t pg_sz = default_page_size) // pg_sz ignored if existing file
      {
        vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_open(p,
          flags::user(flgs), pg_sz);
      }

      // typename vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>:: is required by GCC but not VC++
      typename vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::const_iterator
      insert(const Key& key, const T& mapped_value)
      {
        return vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_non_unique(
          key, mapped_value);
      }

      template <class InputIterator>
      void insert(InputIterator begin, InputIterator end)
      {
        for (; begin != end; ++begin)
          vbtree_base<Key,vbtree_map_base<Key,T,Comp>,Traits,Comp>::m_insert_non_unique(
            begin->key(), begin->mapped_value());
      }
    };


  } // namespace btree
} // namespace boost

#endif  // BOOST_BTREE_MAP_HPP