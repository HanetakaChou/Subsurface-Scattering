/**
 * Copyright (C) 2010 Jorge Jimenez (jorge@iryoku.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the copyright holders.
 */

#include "timer.h"
using namespace std;

Timer::Timer(ID3D11Device* device, ID3D11DeviceContext* context) : enabled(true), flushEnabled(true), windowSize(10), repetitionCount(1)
{
	HRESULT hr;

	D3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_EVENT;
	desc.MiscFlags = 0;
	V(device->CreateQuery(&desc, &event));

	start(context);
}

Timer::~Timer()
{
	SAFE_RELEASE(event);
}

void Timer::start(ID3D11DeviceContext* context)
{
	if (enabled)
	{
		if (flushEnabled)
			flush(context);

		accum = 0.0f;
		QueryPerformanceCounter((LARGE_INTEGER*)&t0);
	}
}

float Timer::clock(ID3D11DeviceContext* context, const wstring& msg)
{
	if (enabled)
	{
		if (flushEnabled)
			flush(context);

		__int64 t1, freq;
		QueryPerformanceCounter((LARGE_INTEGER*)&t1);
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		float t = float(double(t1 - t0) / double(freq));

		float m = mean(msg, t);
		accum += m;

		QueryPerformanceCounter((LARGE_INTEGER*)&t0);

		return m;
	}
	else
	{
		return 0.0f;
	}
}

void Timer::sleep(ID3D11DeviceContext* context, float t)
{
	Sleep(max(int(1000.0f * (t - clock(context))), 0));
}

float Timer::mean(const std::wstring& msg, float t)
{
	Section& section = sections[msg];
	if (windowSize > 1)
	{
		section.buffer.resize(windowSize, make_pair(0.0f, false));
		section.buffer[(section.pos++) % windowSize] = make_pair(t, true);

		section.mean = 0.0;
		float n = 0;
		for (int i = 0; i < int(section.buffer.size()); i++)
		{
			pair<float, bool> val = section.buffer[i];
			if (val.second)
			{
				section.mean += val.first;
				n++;
			}
		}
		section.mean /= n;

		if (section.completed < 1.0f)
			section.completed = float(section.pos - 1) / windowSize;

		return section.mean;
	}
	else
	{
		section.mean = t;
		return section.mean;
	}
}

void Timer::flush(ID3D11DeviceContext* context)
{
	context->End(event);

	BOOL queryData;
	while (context->GetData(event, &queryData, sizeof(BOOL), 0) != S_OK)
	{
		_mm_pause();
	}
}

wostream& operator<<(wostream& out, const Timer& timer)
{
	for (std::map<std::wstring, Timer::Section>::const_iterator section = timer.sections.begin();
		section != timer.sections.end();
		section++)
	{
		const wstring& name = section->first;
		float mean = section->second.mean / timer.repetitionCount;
		float accum = timer.accum / timer.repetitionCount;
		out << name << L" : " << 1000.0f * mean << L"ms : " << int(100.0f * mean / accum) << L"% : " << int(1.0 / mean) << L"fps [" << int(100.0f * section->second.completed) << L"%]" << endl;
	}
	return out;
}
