/*

Copyright (c) 2020, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TORRENT_MERKLE_TREE_HPP_INCLUDED
#define TORRENT_MERKLE_TREE_HPP_INCLUDED

#include "aux_/vector.hpp"
#include "sha1_hash.hpp"
#include "merkle.hpp"
#include <cstdint>

namespace libtorrent {
namespace aux {

// represents the merkle tree for files belonging to a torrent.
// Each file has a root-hash and a "piece layer", i.e. the level in the tree
// representing whole pieces. Those hashes are likely to be included in .torrent
// files and known up-front.

// while downloading, we need to store interior nodes of this tree. However, we
// don't need to store the padding. a SHA-256 is 32 bytes. Instead of storing
// the full (padded) tree of SHA-256 hashes, store the full tree of 32 bit
// signed integers, being indices into the actual storage for the tree. We could
// even grow the storage lazily. Instead of storing the padding hashes, use
// negative indices to refer to fixed SHA-256(0), and SHA-256(SHA-256(0)) and so
// on
struct merkle_tree
{
	merkle_tree() = default;
	merkle_tree(int const num_blocks, char const* root)
		: m_tree(merkle_num_nodes(merkle_num_leafs(num_blocks)))
	{
		m_tree[0] = sha256_hash(root);
	}

	sha256_hash root() const { return m_tree[0]; }

	void load_tree(std::vector<sha256_hash> const& t)
	{
		if (t.empty()) return;
		if (m_tree.empty()) return;
		if (m_tree[0] != t[0]) return;
		if (m_tree.size() != t.size()) return;

		m_tree.assign(t.begin(), t.end());
	}

	int end_index() const { return int(m_tree.size()); }
	span<sha256_hash const> leafs() const
	{
		// given the full size of the tree, the second half of the nodes are
		// leaves, rounded up.
		auto const num_leafs = (m_tree.end_index() + 1) / 2;
		auto const leafs_start = m_tree.end_index() - num_leafs;
		return {&m_tree[leafs_start], num_leafs};
	}

	std::size_t size() const { return m_tree.size(); }
	bool empty() const { return m_tree.empty(); }

	// TODO: all functions that mutate the tree should be members
	sha256_hash& operator[](int const idx) { return m_tree[idx]; }
	sha256_hash const& operator[](int const idx) const { return m_tree[idx]; }

	std::vector<sha256_hash> build_vector() const
	{
		std::vector<sha256_hash> ret(m_tree.begin(), m_tree.end());
		return ret;
	}

	// TODO: maybe these should be constructors
	void fill(int const piece_layer_size)
	{
		merkle_fill_tree(m_tree, piece_layer_size);
	}

	void fill(int const piece_layer_size, int const level_start)
	{
		merkle_fill_tree(m_tree, piece_layer_size, level_start);
	}

	void clear(int num_leafs, int level_start)
	{
		merkle_clear_tree(m_tree, num_leafs, level_start);
	}

private:
	aux::vector<sha256_hash> m_tree;
};

}
}

#endif
