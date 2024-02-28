/* Copyright (c) 2017, 2021 Pieter Wuille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <vector>
#include <string>
#include<algorithm>

namespace payment_request
{

/** Decode a Lightning Payment Request -1 means failure. */
std::pair<int, std::vector<uint8_t> > decode(const std::string& addr);


struct bolt11 {
	std::string prefix;
	uint64_t timestamp;
	uint64_t sat_amount;

	unsigned char payment_hash[32];
	unsigned char receiver_id[33];

	/* description_hash valid if and only if description is NULL. */
	unsigned char description[639];                 // Note that the maximum length of a Tagged Field's data is constricted by the maximum value of data_length. This is 1023 x 5 bits, or 639 bytes.
	unsigned char description_hash[32];

	/* How many seconds to pay from @timestamp above. */
	uint64_t expiry;

	/* How many blocks final hop requires. */
	uint32_t min_final_cltv_expiry;

	/* If non-NULL, indicates fallback addresses to pay to. */
	//const u8 **fallbacks;

	/* If non-NULL: array of route arrays */
	//struct route_info **routes;

	/* signature of sha256 of entire thing. */
	unsigned char sig[64];

	/* payment secret, if any. */
	unsigned char payment_secret[32];

	/* Features bitmap, if any. */
	//u8 *features;

	/* Optional metadata to send with payment. */
	//u8 *metadata;

	//struct list_head extra_fields;
};

}