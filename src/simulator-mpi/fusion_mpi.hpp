// Copyright 2017 ProjectQ-Framework (www.projectq.ch)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FUSION_MPI_HPP
#define FUSION_MPI_HPP

#include <glog/logging.h>

#include <algorithm>
#include <boost/format.hpp>
#include <complex>
#include <iostream>
#include <set>
#include <vector>

#include "alignedallocator.hpp"
#include "funcs.hpp"

class Item
{
public:
     using Index = int64_t;
     using IndexVector = std::vector<Index>;
     using Complex = std::complex<double>;
     using Matrix
         = std::vector<std::vector<Complex, aligned_allocator<Complex, 64>>>;

     Item(Matrix mat, uint64_t aFlags, IndexVector idx)
         : mat_(mat), idx_(idx), m_flags(aFlags)
     {}

     ~Item()
     {}

     Matrix& get_matrix()
     {
          return mat_;
     }
     IndexVector& get_indices()
     {
          return idx_;
     }
     uint64_t flags()
     {
          return m_flags;
     }

     Matrix mat_;
     IndexVector idx_;
     uint64_t m_flags;
};

class Fusion
{
public:
     using Index = int64_t;
     using IndexSet = std::set<Index>;
     using IndexVector = std::vector<Index>;
     using Complex = std::complex<double>;
     using Matrix
         = std::vector<std::vector<Complex, aligned_allocator<Complex, 64>>>;
     using ItemVector = std::vector<Item>;

     unsigned num_qubits()
     {
          return set_.size();
     }

     std::size_t size() const
     {
          return items_.size();
     }

     void insert(Matrix matrix, uint64_t flags, IndexVector index_list,
                 IndexVector const& ctrl_list = {})
     {
          if (matrix.size() == 1) {
               factor *= matrix[0][0];
               return;
          }

          for (auto idx: index_list)
               set_.emplace(idx);

          handle_controls(matrix, index_list, ctrl_list);
          Item item(matrix, flags, index_list);
          items_.push_back(item);
     }

     void perform_fusion(Matrix& fused_matrix, IndexVector& index_list,
                         IndexVector& ctrl_list, uint64_t& out_flags)
     {
          index_list.insert(index_list.end(), set_.begin(), set_.end());

          std::size_t N = num_qubits();
          fused_matrix = Matrix(
              1UL << N,
              std::vector<Complex, aligned_allocator<Complex, 64>>(1UL << N));
          auto& M = fused_matrix;

          for (std::size_t i = 0; i < (1UL << N); ++i)
               M[i][i] = factor;

          DLOG(INFO)
              << boost::format(
                     "perform_fusion(): items_.size(): %d, num qubits: %d")
                     % items_.size() % N;
          out_flags |= MatProps::IS_DIAG;
          for (auto& item: items_) {
               uint64_t flags = item.flags();
               if (!(flags & MatProps::IS_DIAG))
                    out_flags &= ~MatProps::IS_DIAG;

               auto const& idx = item.get_indices();
               IndexVector idx2mat(idx.size());
               for (std::size_t i = 0; i < idx.size(); ++i)
                    idx2mat[i] = ((std::equal_range(index_list.begin(),
                                                    index_list.end(), idx[i]))
                                      .first
                                  - index_list.begin());

               for (std::size_t k = 0; k < (1UL << N);
                    ++k) {  // loop over big matrix columns
                    // check if column index satisfies control-mask
                    // if not: leave it unchanged
                    std::vector<Complex> oldcol(1UL << N);
                    for (std::size_t i = 0; i < (1UL << N); ++i)
                         oldcol[i] = M[i][k];

                    for (std::size_t i = 0; i < (1UL << N); ++i) {
                         std::size_t local_i = 0;
                         for (std::size_t l = 0; l < idx.size(); ++l)
                              local_i |= ((i >> idx2mat[l]) & 1UL) << l;

                         Complex res = 0.;
                         for (std::size_t j = 0; j < (1UL << idx.size()); ++j) {
                              std::size_t locidx = i;
                              for (std::size_t l = 0; l < idx.size(); ++l)
                                   if (((j >> l) & 1UL)
                                       != ((i >> idx2mat[l]) & 1UL))
                                        locidx ^= (1UL << idx2mat[l]);
                              res += oldcol[locidx]
                                     * item.get_matrix()[local_i][j];
                         }
                         M[i][k] = res;
                    }
               }
          }
          ctrl_list.reserve(ctrl_set_.size());
          for (auto ctrl: ctrl_set_)
               ctrl_list.push_back(ctrl);

          factor = 1.0;
     }

     static void add_controls(Matrix& matrix, IndexVector& indexList,
                              IndexVector const& new_ctrls)
     {
          indexList.reserve(indexList.size() + new_ctrls.size());
          indexList.insert(indexList.end(), new_ctrls.begin(), new_ctrls.end());

          std::size_t F = (1UL << new_ctrls.size());
          Matrix newmatrix(F * matrix.size(),
                           std::vector<Complex, aligned_allocator<Complex, 64>>(
                               F * matrix.size(), 0.));

          std::size_t Offset = newmatrix.size() - matrix.size();

          for (std::size_t i = 0; i < Offset; ++i)
               newmatrix[i][i] = 1.;
          for (std::size_t i = 0; i < matrix.size(); ++i) {
               for (std::size_t j = 0; j < matrix.size(); ++j)
                    newmatrix[Offset + i][Offset + j] = matrix[i][j];
          }
          matrix = std::move(newmatrix);
     }

private:
     void handle_controls(Matrix& matrix, IndexVector& indexList,
                          IndexVector const& ctrlList)
     {
          auto unhandled_ctrl = ctrl_set_;  // will contain all ctrls that are
                                            // not part of the new command
          // --> need to be removed from the global mask and the controls
          // incorporated into the old commands (the ones already in the list).

          for (auto ctrlIdx: ctrlList) {
               if (ctrl_set_.count(ctrlIdx)
                   == 0) {  // need to either add it to the list or to the
                            // command
                    if (items_.size() > 0) {  // add it to the command
                         add_controls(matrix, indexList, {ctrlIdx});
                         set_.insert(ctrlIdx);
                    }
                    else  // add it to the list
                         ctrl_set_.emplace(ctrlIdx);
               }
               else
                    unhandled_ctrl.erase(ctrlIdx);
          }
          // remove global controls which are no longer global (because the
          // current command didn't have it)
          if (unhandled_ctrl.size() > 0) {
               IndexVector new_ctrls;
               new_ctrls.reserve(unhandled_ctrl.size());
               for (auto idx: unhandled_ctrl) {
                    new_ctrls.push_back(idx);
                    ctrl_set_.erase(idx);
                    set_.insert(idx);
               }
               for (auto& item: items_)
                    add_controls(item.get_matrix(), item.get_indices(),
                                 new_ctrls);
          }
     }

public:
     IndexSet set_;
     ItemVector items_;
     IndexSet ctrl_set_;

     Complex factor = 1.0;
};

#endif
