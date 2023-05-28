#pragma once

#include "../common/Log.h"
#include <assert.h>

namespace yoss
{
    
    //-----------------------------------------------------------------------
    template <class T> class CircularSummedBuffer
    {
    public:
        typedef int Timestamp;
        typedef int BackPos;
        
        //-----------------------------------------------------------------------
        CircularSummedBuffer(int size, bool initially_full) :
            _size(size),
            _currentPos(-1),
            _currentTimestamp(-1),
            _occupied(initially_full ? size : 0)
        {
            ASSERT(size > 0);
            _buffer = new T[_size];
        }

        //-----------------------------------------------------------------------
        CircularSummedBuffer()
        {
            delete[] _buffer;
        }
        
        //-----------------------------------------------------------------------
        void FillWith(const T& value)
        {
            _occupied = _size;
            
            _buffer[GetAbsPos(_occupied - 1)] = value;
            
            for (int i = _occupied - 2; i >= 0; i--)
                _buffer[GetAbsPos(i)] = _buffer[GetAbsPos(i + 1)] + value;
        }
        
        //-----------------------------------------------------------------------
        void Push(const T& value)
        {
            if (_occupied < _size)
                _occupied++;
            
            _currentTimestamp++;
            _currentPos++;
            if (_currentPos >= _size)
                _currentPos = 0;
            
            if (_currentPos > 0 && _occupied > 0)
                _buffer[_currentPos] = _buffer[GetAbsPos(1)] + value;
            else
                _buffer[_currentPos] = value;
        }
        
        //-----------------------------------------------------------------------
        void AddToLast(const T& value)
        {
            _buffer[_currentPos] += value;
        }
        
        //-----------------------------------------------------------------------
        T Pop()
        {
            ASSERT(_occupied > 0);
            
            T value = Get();
            
            _occupied--;
            _currentTimestamp--;
            _currentPos--;
            if (_currentPos < 0)
                _currentPos = _size - 1;
            
            return value;
        }
        
        //-----------------------------------------------------------------------
        T Get(BackPos back_pos = 0) const
        {
            ASSERT(IsBackPosOccupied(back_pos));
            
            int pos = GetAbsPos(back_pos);
            
            if (pos > 0 && IsBackPosOccupied(back_pos + 1))
                return _buffer[pos] - _buffer[GetAbsPos(back_pos + 1)];
            else
                return _buffer[pos];
        }
        
        //-----------------------------------------------------------------------
        T GetByTimestamp(Timestamp timestamp) const
        {
            BackPos back_pos = GetBackPos(timestamp);
            
            return Get(back_pos);
        }
        
        //-----------------------------------------------------------------------
        T GetDiff(BackPos back_pos = 0) const
        {
            ASSERT(IsBackPosOccupied(back_pos));
            
            auto val = Get(back_pos);
            if (IsBackPosOccupied(back_pos + 1))
            {
                T val_prev = Get(back_pos + 1);
                return val - val_prev;
            }
            return val;
        }
        
        //-----------------------------------------------------------------------
        T GetSum(BackPos from_back_pos, BackPos to_back_pos) const
        {
            ASSERT(from_back_pos <= to_back_pos);
            ASSERT(to_back_pos < _occupied);
            
            int from_abs_pos = GetAbsPos(from_back_pos);
            T sum = _buffer[from_abs_pos];
            
            int to_abs_pos = GetAbsPos(to_back_pos);
            if (to_abs_pos > from_abs_pos)
            {
                sum += _buffer[_size - 1];
            }
            
            if (IsBackPosOccupied(to_back_pos + 1) && to_abs_pos > 0)
                sum -= _buffer[GetAbsPos(to_back_pos + 1)];
            
            return sum;
        }
        
        //-----------------------------------------------------------------------
        T GetAverage(BackPos from_back_pos, BackPos to_back_pos) const
        {
            double one_div_elements_num = (double)1 / (double)(to_back_pos - from_back_pos + 1);
            T avg = GetSum(from_back_pos, to_back_pos) * one_div_elements_num;
            return avg;
        }
        
        //-----------------------------------------------------------------------
        BackPos GetBackPos(Timestamp timestamp) const
        {
            return(BackPos)(_currentTimestamp - timestamp);
        }
        
        //-----------------------------------------------------------------------
        Timestamp GetTimestamp(BackPos back_pos = 0) const
        {
            return _currentTimestamp - (Timestamp)back_pos;
        }
        
        //-----------------------------------------------------------------------
        bool IsStillInBuffer(Timestamp timestamp) const
        {
            ASSERT(timestamp <= _currentTimestamp);
            return ((int)(_currentTimestamp - timestamp) < _occupied);
        }
        
        //-----------------------------------------------------------------------
        bool IsBackPosOccupied(BackPos back_pos) const
        {
            ASSERT(back_pos >= 0);
            return ((int)back_pos < _occupied);
        }
        
        //-----------------------------------------------------------------------
        int GetOccupiedSize() const
        {
            return _occupied;
        }
        
        
    protected:
        int _size;
        int _occupied;
        T* _buffer;
        
        int _currentPos;
        Timestamp _currentTimestamp;
        
        //-----------------------------------------------------------------------
        int GetAbsPos(BackPos back_pos) const
        {
            ASSERT(IsBackPosOccupied(back_pos));
            
            int pos = (_currentPos - (int)back_pos);
            if (pos < 0)
                pos += _size;
            
            return pos;
        }
    };
    
}

