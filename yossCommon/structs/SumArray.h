#pragma once

#include "../common/Log.h"
#include "../graphics/Graphics.h"
#include <vector>
#include <string>


//----------------------------------------------------------------------------------
template <class SumValue>
class SumArray
//----------------------------------------------------------------------------------
{
public:
	//----------------------------------------------------------------------------------
	SumArray(int dimensions_num, int slots_per_unit, int* size) :
		m_dimensionsNum(dimensions_num),
		m_slotsNum(slots_per_unit)
	{
		// Create m_sizes
		int map_pixels = 1;
		m_sizes = new int[dimensions_num];
		for (int dim = 0; dim < m_dimensionsNum; dim++)
		{
			m_sizes[dim] = size[dim];
			map_pixels *= size[dim];
		}

		// Create m_arrays
		m_arrays = new SumValue*[m_slotsNum];
		for (int slot = 0; slot < m_slotsNum; slot++)
		{
			m_arrays[slot] = new SumValue[map_pixels];
			memset((void*)m_arrays[slot], 0, map_pixels * sizeof(SumValue));
		}
	}

	//----------------------------------------------------------------------------------
	~SumArray()
	{
		delete[] m_sizes;

		for (int slot = 0; slot < m_slotsNum; slot++)
			delete[] m_arrays[slot];
		delete[] m_arrays;
	}

	inline int GetSlotsNum() const { return m_slotsNum; }
	inline int GetDimensionsNum() const { return m_dimensionsNum; }
	inline int GetArraySize(int dimension_index) const { return m_sizes[dimension_index]; }
	
	inline SumValue** GetInternalArrays() const { return m_arrays; }

	//----------------------------------------------------------------------------------
	bool GenerateFromImage(void* data, int w, int h, int bpp)
	{
		ASSERT(m_dimensionsNum == 2);
		ASSERT(w <= m_sizes[0]);
		ASSERT(h <= m_sizes[1]);

		int* image_pixels = (int*)data;
		SumValue* array_r = m_arrays[0];
		SumValue* array_g = m_arrays[1];
		SumValue* array_b = m_arrays[2];
		const int array_w = m_sizes[0];
		int index_y = 0;

		for (int y = 0; y < h; y++)
		{
			int index_xy = index_y;

			for (int x = 0; x < w; x++)
			{
				int pixel = image_pixels[index_xy];

				SumValue value_r = (SumValue)((pixel & 0xFF0000) >> 16);
				SumValue value_g = (SumValue)((pixel & 0xFF00) >> 8);
				SumValue value_b = (SumValue)((pixel & 0xFF));

				if (x > 0)
				{
					value_r += array_r[index_xy - 1];
					value_g += array_g[index_xy - 1];
					value_b += array_b[index_xy - 1];
				}
				if (y > 0)
				{
					value_r += array_r[index_xy - array_w];
					value_g += array_g[index_xy - array_w];
					value_b += array_b[index_xy - array_w];
				}
				if (x > 0 && y > 0)
				{
					value_r -= array_r[index_xy - array_w - 1];
					value_g -= array_g[index_xy - array_w - 1];
					value_b -= array_b[index_xy - array_w - 1];
				}

				array_r[index_xy] = value_r;
				array_g[index_xy] = value_g;
				array_b[index_xy] = value_b;

				index_xy++;
			}

			index_y += array_w;
		}

		return true;
	}

	//----------------------------------------------------------------------------------
	inline SumValue GetPlotValue(int slot_index, int* coo1, int* coo2, bool return_average)
	{
		SumValue* slot_array = m_arrays[slot_index];

		if (m_dimensionsNum == 1)
		{
			SumValue plot_value = slot_array[*coo2];
			if (*coo1 > 0)
				plot_value -= slot_array[*coo1 - 1];
			if (return_average)
				plot_value /= *coo2 - *coo1 + 1;
			return plot_value;
		}
		else if (m_dimensionsNum == 2)
		{
			int &x1 = coo1[0]; int &y1 = coo1[1];
			int &x2 = coo2[0]; int &y2 = coo2[1];
			int scanline_y2 = y2 * m_sizes[0];
			int scanline_y1m1 = (y1 - 1) * m_sizes[0];

			SumValue plot_value = slot_array[scanline_y2 + x2];
			if (x1 > 0) plot_value -= slot_array[scanline_y2 + x1 - 1];
			if (y1 > 0) plot_value -= slot_array[scanline_y1m1 + x2];
			if (x1 > 0 && y1 > 0)
				plot_value += slot_array[scanline_y1m1 + x1 - 1];

			if (return_average)
			{
				int plot_area = (x2 - x1 + 1) * (y2 - y1 + 1);
				plot_value /= (SumValue)plot_area;
			}

			return plot_value;
		}
		else ASSERT(false);
		return -1;
	}

	//----------------------------------------------------------------------------------
	inline SumValue GetPlotValue(int slot_index, int x1, int y1, int x2, int y2, bool return_average)
	{
		int coo1[2] = { x1, y1 };
		int coo2[2] = { x2, y2 };
		return GetPlotValue(slot_index, coo1, coo2, return_average);
	}

	//----------------------------------------------------------------------------------
    inline SumValue GetPlotValue(int slot_index, const yoss::graphics::Rect2D& rect, bool return_average)
	{
        int coo1[2] = { (int)rect.min.x, (int)rect.min.y };
        int coo2[2] = { (int)rect.max.x, (int)rect.max.y };
        return GetPlotValue(slot_index, coo1, coo2, return_average);
	}
	
	//----------------------------------------------------------------------------------
	inline yoss::graphics::Color GetPlotColor(int* coo1, int* coo2)
	{
		SumValue val_r, val_g, val_b;
		int plot_size;

		if (m_dimensionsNum == 1)
		{
			plot_size = *coo2 - *coo1 + 1;

			val_r = m_arrays[0][*coo2];
			val_g = m_arrays[1][*coo2];
			val_b = m_arrays[2][*coo2];

			if (*coo1 > 0)
			{
				val_r -= m_arrays[0][*coo1 - 1];
				val_g -= m_arrays[1][*coo1 - 1];
				val_b -= m_arrays[2][*coo1 - 1];
			}
		}
		else if (m_dimensionsNum == 2)
		{
			int &x1 = coo1[0]; int &y1 = coo1[1];
			int &x2 = coo2[0]; int &y2 = coo2[1];
			int scanline_y2 = y2 * m_sizes[0];
			int scanline_y1m1 = (y1 - 1) * m_sizes[0];

			plot_size = (x2 - x1 + 1) * (y2 - y1 + 1);

			val_r = m_arrays[0][scanline_y2 + x2];
			val_g = m_arrays[1][scanline_y2 + x2];
			val_b = m_arrays[2][scanline_y2 + x2];

			if (x1 > 0)
			{
				int addr_y2x1m1 = scanline_y2 + x1 - 1;
				val_r -= m_arrays[0][addr_y2x1m1];
				val_g -= m_arrays[1][addr_y2x1m1];
				val_b -= m_arrays[2][addr_y2x1m1];
			}
			if (y1 > 0)
			{
				int addr_y1m1x2 = scanline_y1m1 + x2;
				val_r -= m_arrays[0][addr_y1m1x2];
				val_g -= m_arrays[1][addr_y1m1x2];
				val_b -= m_arrays[2][addr_y1m1x2];
			}
			if (x1 > 0 && y1 > 0)
			{
				int addr_y1m1x1m1 = scanline_y1m1 + x1 - 1;
				val_r += m_arrays[0][addr_y1m1x1m1];
				val_g += m_arrays[1][addr_y1m1x1m1];
				val_b += m_arrays[2][addr_y1m1x1m1];
			}
		}
		else
		{
			ASSERT(false);
			return yoss::graphics::COLOR_BLACK;
		}

		val_r /= (SumValue)plot_size;
		val_g /= (SumValue)plot_size;
		val_b /= (SumValue)plot_size;

		return yoss::graphics::Color((yoss::graphics::ColorC)val_r,
				(yoss::graphics::ColorC)val_g,
				(yoss::graphics::ColorC)val_b,
                yoss::graphics::COLORC_MAX);
	}

	//----------------------------------------------------------------------------------
	inline yoss::graphics::Color GetPlotColor(int x1, int y1, int x2, int y2)
	{
		int coo1[2] = { x1, y1 };
		int coo2[2] = { x2, y2 };
		return GetPlotColor(coo1, coo2);
	}

	//----------------------------------------------------------------------------------
	inline yoss::graphics::Color GetPlotColor(const yoss::graphics::Rect2D& rect)
	{
        int coo1[2] = { (int)rect.min.x, (int)rect.min.y };
        int coo2[2] = { (int)rect.max.x, (int)rect.max.y };
        return GetPlotColor(coo1, coo2);
	}

	//----------------------------------------------------------------------------------
	inline SumValue GetPixelValue(int slot_index, int* coo)
	{
		if (m_dimensionsNum == 1)
			return m_arrays[slot_index][*coo];
		else if (m_dimensionsNum == 2)
			return m_arrays[slot_index][coo[1] * m_sizes[0] + coo[0]];
		else ASSERT(false);
		return (SumValue)-1;
	}

	//----------------------------------------------------------------------------------
	// Note: Calling SumArray::SetPixelValue(x, y) requires that you have set all values for [0 <= x <= coo[0] ; 0 <= y <= coo[1]] before that!!!
	inline void SetPixelValue(int slot_index, int* coo, SumValue value)
	{
		static const int coo_zero[2] = { 0, 0 };

		if (m_dimensionsNum == 1)
			m_arrays[slot_index][*coo] = GetPlotValue(slot_index, (int*)coo_zero, coo, false) - m_arrays[slot_index][*coo] + value;
		else if (m_dimensionsNum == 2)
		{
			SumValue* slot_array = m_arrays[slot_index];
			int &x = coo[0];
			int &y = coo[1];
			int index_y = y * m_sizes[0];
			int index_xy = index_y + x;

			slot_array[index_xy] = value;
			if (x > 0) slot_array[index_xy] += slot_array[index_xy - 1];
			if (y > 0) slot_array[index_xy] += slot_array[index_xy - m_sizes[0]];
			if (x > 0 && y > 0)
				slot_array[index_xy] -= slot_array[index_xy - m_sizes[0] - 1];
		}
		else ASSERT(false);
	}


protected:

	const int m_slotsNum; // Number of sum maps
	const int m_dimensionsNum; // Number of dimensions of each sum map
	int* m_sizes; // Array with sizes (for each dimension)
	SumValue** m_arrays;
};
