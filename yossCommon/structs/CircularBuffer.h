#pragma once

#include "../common/Log.h"


namespace yoss
{

    //-----------------------------------------------------------------------
    template <class T> class CircularBuffer
    {
    public:
        typedef int Timestamp;
        typedef int BackPos;
        
        //-----------------------------------------------------------------------
        CircularBuffer(int size, bool initially_full) :
            _size(size),
            _currentPos(-1),
            _currentTimestamp(-1),
            _occupied(initially_full ? size : 0)
        {
            ASSERT(size > 0);
            _buffer = new T[_size];
        }

        //-----------------------------------------------------------------------
        ~CircularBuffer()
        {
            delete[] _buffer;
        }
        
        //-----------------------------------------------------------------------
        void FillWith(const T& value)
        {
            _occupied = _size;
            for (int i = 0; i < _occupied; i++)
                _buffer[i] = value;
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
                _currentPos += _size;
            
            return value;
        }
        
        //-----------------------------------------------------------------------
        T& Get(BackPos back_pos = 0) const
        {
            ASSERT(IsBackPosOccupied(back_pos));
            
            int pos = GetAbsPos(back_pos);
            return _buffer[pos];
        }
        
        //-----------------------------------------------------------------------
        T GetAverage(BackPos from_back_pos, BackPos to_back_pos) const
        {
            T sum = 0;
            for (BackPos i = from_back_pos; i <= to_back_pos; i++)
                sum += Get(i);
            
            double one_div_elements_num = (double)1 / (double)(to_back_pos - from_back_pos + 1);
            T avg = (sum *= one_div_elements_num);
            return avg;
        }
        
        //-----------------------------------------------------------------------
        T* GetNext(T* current_T) const
        {
            ASSERT(current_T);
            ASSERT(current_T >= _buffer && current_T < _buffer + _size);
            int abs_pos = (int)(current_T - _buffer);
            ASSERT(abs_pos >= 0 && abs_pos < _size);
            BackPos back_pos = GetBackPosFromAbsPos(abs_pos);
            
            if (back_pos > 0)
                return &Get(back_pos - 1);
            else
                return nullptr;
        }
        
        //-----------------------------------------------------------------------
        T* GetPrev(T* current_T) const
        {
            ASSERT(current_T);
            ASSERT(current_T >= _buffer && current_T < _buffer + _size);
            int abs_pos = (int)(current_T - _buffer);
            BackPos back_pos = GetBackPosFromAbsPos(abs_pos);
            
            if (back_pos < _occupied - 1)
                return &Get(back_pos + 1);
            else
                return nullptr;
        }
        
        //-----------------------------------------------------------------------
        T& GetByTimestamp(Timestamp timestamp) const
        {
            BackPos back_pos = GetBackPos(timestamp);
            
            return Get(back_pos);
        }
        
        //-----------------------------------------------------------------------
        T GetDiff(BackPos back_pos = 0) const
        {
            ASSERT(IsBackPosOccupied(back_pos));
            
            return Get(back_pos) - (IsBackPosOccupied(back_pos + 1) ? Get(back_pos + 1) : 0);
        }
        
        //-----------------------------------------------------------------------
        BackPos GetBackPos(Timestamp timestamp) const
        {
            return(BackPos)(_currentTimestamp - timestamp);
        }
        
        //-----------------------------------------------------------------------
        BackPos GetBackPos(T& t) const
        {
            ASSERT(&t >= _buffer && &t < _buffer + _size);
            int index_in_buff = (&t - _buffer);
            ASSERT(index_in_buff >= 0 && index_in_buff < _occupied);
            if (index_in_buff <= _currentPos)
                return (_currentPos - index_in_buff);
            else
                return (_currentPos + _size - index_in_buff);
        }
        
        //-----------------------------------------------------------------------
        Timestamp GetTimestamp(BackPos back_pos = 0) const
        {
            return _currentTimestamp - (Timestamp)back_pos;
        }
        
        //-----------------------------------------------------------------------
        bool IsStillInBuffer(Timestamp timestamp) const
        {
            return ((int)(_currentTimestamp - timestamp) < _occupied);
        }
        
        //-----------------------------------------------------------------------
        bool IsBackPosOccupied(BackPos back_pos) const
        {
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
        
        //-----------------------------------------------------------------------
        BackPos GetBackPosFromAbsPos(int abs_pos) const
        {
            BackPos back_pos = (BackPos)(_currentPos - abs_pos);
            if (back_pos < 0)
                back_pos += _size;
            
            //ASSERT(IsBackPosOccupied(back_pos));
            
            return back_pos;
        }
    };
    
}

