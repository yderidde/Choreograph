/*
 * Copyright (c) 2014 David Wicks, sansumbrella.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Source.h"
#include "Phrase.hpp"
#include <cmath>

namespace choreograph
{

/**
 A Sequence of motions.
 Our essential compositional tool, describing all the transformations to one element.
 A kind of platonic idea of an animation sequence; this describes a motion without giving it an output.
*/
template<typename T>
class Sequence : public Source<T>
{
public:
  // Sequences always need to have some valid value.
  Sequence() = delete;
  using SequenceT = Sequence<T>;

  /// Construct a Sequence with an initial \a value.
  explicit Sequence( const T &value ):
    Source<T>( 0, 0 ),
    _initial_value( value )
  {}

  /// Construct a Sequence from an array of Phrases.
  explicit Sequence( const std::vector<SourceRef<T>> &phrases ):
    Source<T>( phrases.front()->getStartTime(), phrases.back()->getEndTime() ),
    _initial_value( phrases.front()->getStartValue() ),
    _phrases( phrases )
  {}

  /// Returns the Sequence value at \a atTime.
  T getValue( float atTime ) const override;

  /// Set current value. An instantaneous hold.
  SequenceT& set( const T &value )
  {
    if( _phrases.empty() ) {
      _initial_value = value;
    }
    else {
      then<Hold>( value, 0.0f );
    }
    return *this;
  }

  /// Returns a copy of this sequence. Useful if you want to make a base animation and modify that.
  std::shared_ptr<SequenceT> copy() const { return std::make_shared<SequenceT>( *this ); }

  /// Add a phrase to the end of the sequence.
  /// All phrases will receive the following arguments to their constructors:
  /// start time, end time, start value (last sequence value), end value
  /// If additional arguments are passed to then(), those arguments come after the required ones.
  /// sequence.then<RampTo>( targetValue, duration, other phrase parameters ).then<Hold>( holdValue, duration );
  template<template <typename> class PhraseT, typename... Args>
  SequenceT& then( const T &value, float duration, Args... args )
  {
    auto phrase = std::make_shared<PhraseT<T>>( this->getEndTime(), this->getEndTime() + duration, this->getEndValue(), value, args... );
    _phrases.push_back( phrase );
    this->setEndTime( phrase->getEndTime() );

    return *this;
  }

  /// Returns the value at the end of the Sequence.
  T getEndValue() const override { return _phrases.empty() ? _initial_value : _phrases.back()->getEndValue(); }

  /// Returns the value at the beginning of the Sequence.
  T getStartValue() const override { return _initial_value; }

  /// Returns the number of phrases in the Sequence.
  size_t getPhraseCount() const { return _phrases.size(); }

  /// TODO: implement sequence concatenation.
  static SequenceT& concatenate( const SequenceT &lhs, const SequenceT &rhs );

  /// Recursively concatenate any number of sequences.
  template<typename... Args>
  static SequenceT& concatenate( const SequenceT &first, const SequenceT &second, Args... additional )
  {
    return concatenate( concatenate( first, second ), std::forward( additional... ) );
  }
private:
  std::vector<SourceRef<T>> _phrases;
  T                         _initial_value;
};

/// Returns the value of this sequence for a given point in time.
// Would be nice to have a constant-time check (without a while loop).
template<typename T>
T Sequence<T>::getValue( float atTime ) const
{
  if( atTime < this->getStartTime() )
  {
    return _initial_value;
  }
  else if ( atTime >= this->getEndTime() )
  {
    return getEndValue();
  }

  auto iter = _phrases.begin();
  while( iter < _phrases.end() ) {
    if( (*iter)->getEndTime() > atTime )
    {
      return (*iter)->getValue( atTime );
    }
    ++iter;
  }
  // past the end, get the final value
  // this should be unreachable, given that we return early if time >= duration
  return getEndValue();
}

template<typename T>
using SequenceRef = std::shared_ptr<Sequence<T>>;

} // namespace choreograph