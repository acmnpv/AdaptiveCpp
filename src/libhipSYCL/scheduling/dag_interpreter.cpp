/*
 * This file is part of hipSYCL, a SYCL implementation based on CUDA/HIP
 *
 * Copyright (c) 2019 Aksel Alpay
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <cassert>

#include "CL/sycl/detail/scheduling/dag_interpreter.hpp"
#include "CL/sycl/detail/scheduling/hints.hpp"

namespace cl {
namespace sycl {
namespace detail {

namespace {

void add_requirements_from_node_to(const dag_node_ptr &node,
                                   std::vector<dag_node_ptr> &target) 
{
  for(auto req : node->get_requirements())
    target.push_back(req);
}

}

dag_interpreter::dag_interpreter(const dag *d, const dag_enumerator *enumerator,
                                 const dag_expansion_result *expansion_result)
    : _expansion{expansion_result},
      _effective_requirements(enumerator->get_node_index_space_size()) 
{
  d->for_each_node([this](dag_node_ptr node) {
    std::size_t node_id = this->get_node_id(node);
    
    const dag_expander_annotation &node_annotation =
        _expansion->node_annotations(node_id);

    // If this is a forwarded node, add the requirements to
    // the forwarding target node
    if(node_annotation.is_node_forwarded()){
      std::size_t forwarded_id =
          get_node_id(node_annotation.get_forwarding_target());

      add_requirements_from_node_to(node,
                                    this->_effective_requirements[forwarded_id]);
      }
    }
    else {
      add_requirements_from_node_to(node,
                                    this->_effective_requirements[node_id]);
    }
    // TODO requirements from nodes that are optimized away must be pulled into
    // the parent nodes
    
  });
}

bool dag_interpreter::is_node_real(const dag_node_ptr &node) const
{
  std::size_t node_id = this->get_node_id(node);

  return !_expansion->node_annotations(node_id).is_optimized_away();
}

std::size_t dag_interpreter::get_node_id(const dag_node_ptr &node) const
{
  assert(node->get_execution_hints().has_hint(
      execution_hint_type::dag_enumeration_id));

  return node->get_execution_hints()
      .get_hint<hints::dag_enumeration_id>()
      ->id();
}

}
}
}