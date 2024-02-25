/* Copyright (c) 2023 Marcello Pinsdorf
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
#include "payment_request.h"
#include "bech32.h"

#include <tuple>

namespace
{

typedef std::vector<uint8_t> data;

/** Convert from one power-of-2 number base to another. */
bool convertbits(bool pad, int frombits, int tobits, data& out, const data& in) {
    int acc = 0;
    int bits = 0;
    const int maxv = (1 << tobits) - 1;
    const int max_acc = (1 << (frombits + tobits - 1)) - 1;
    for (size_t i = 0; i < in.size(); ++i) {
        int value = in[i];
        acc = ((acc << frombits) | value) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) {
        if (bits) out.push_back((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

/* Helper for pulling a variable-length big-endian int. */
bool pull_uint(const data& in,uint64_t* val, size_t databits)
{
    data out;
    int tobits = 8;
    int frombits = 5;
    *val = 0;
    /* Padd if size variable is not multiple of tobits */
    bool need_padding = (in.size() * tobits) % tobits == 0;
    /* Conver frombits base tobits base*/    
    if (!convertbits(need_padding, frombits, tobits, out, in)) return false;
    /* take the tobits base bytes and calculate the decima value */
    for (long unsigned int i = 0; i < out.size(); ++i)
        *val |= static_cast<uint64_t>(out[i]) << (8 * (out.size()-i-1));
    /* Adjust the shifitng to databits value*/
    *val = *val >> (out.size() * 8 - databits);    
    return true;
}

}

namespace payment_request
{

std::vector<std::string> prefix_list = {"lnbc", "lntb", "lntbs", "lnbcrt"};

/** Decode a Lightning Payment Request **/
std::pair<int, data> decode(const std::string& invoice) {
    const auto dec = bech32::decode(invoice);
    if (dec.data.size() < 1) return std::make_pair(-1, data());
    /* BOLT11 - if it does NOT understand the prefix MUST fail the payment. */
    /* First find the invoice value field into hrp */
    std::string hrp_value, hrp_prefix;
    for (size_t i = 0; i < dec.hrp.size(); ++i) {
        if (isdigit(dec.hrp[i])) {
            hrp_value = dec.hrp.substr(i, dec.hrp.size()-1 );
            hrp_prefix = dec.hrp.substr(0, i);
            break;
        }
    }
    if(hrp_prefix == ""){                   /* If there is no value for the invoice */
        hrp_prefix = dec.hrp;
    } else {
    /* Get the numeric value of the value hrp */
        size_t invoice_value = std::stoul(hrp_value.substr(0,hrp_value.size()-1));
        /*  m (milli): multiply by 0.001
            u (micro): multiply by 0.000001
            n (nano): multiply by 0.000000001
            p (pico): multiply by 0.000000000001

            Value will be calculated in msats
        */
        if(isdigit(hrp_value[hrp_value.size()-1])){
            invoice_value = invoice_value*100000000*1000;                       // There is no multiplier
        } else {
            switch (hrp_value[hrp_value.size()-1]) {
                case 'm':
                    invoice_value = invoice_value*100000000;                    //*0.001*100000000*1000;
                    break;
                case 'u':
                    invoice_value = invoice_value*100000;                       //*0.000001*100000000*1000;
                    break;
                case 'n':
                    invoice_value = invoice_value*100;                          //*0.000000001*100000000*1000;
                    break;
                case 'p':
                    invoice_value = invoice_value*0.1;                          //*0.000000000001*100000000*1000;
                    break;
                default:
                    return std::make_pair(-1, data());
                    break;
            }
        }
    }
    /* if it does NOT understand the prefix MUST fail the payment.*/
    if(std::find(prefix_list.begin(), prefix_list.end(), hrp_prefix) == prefix_list.end())
        return std::make_pair(-1, data());
    /* Take the timesatamp of the invoice*/
    uint64_t timestamp = 0;
    if(!pull_uint(data(dec.data.begin(), dec.data.begin()+7), &timestamp, 35)) return std::make_pair(-1, data());;

    uint64_t data_lenght = 0;
    data conv, preimage, secret;
    int tobits = 8;
    int frombits = 5;
    int data_part_pointer = 7;
    do {
        conv = {};
        data_part_pointer += data_lenght;
        if(!pull_uint(data(dec.data.begin() + data_part_pointer + 1, dec.data.begin() + data_part_pointer + 3), &data_lenght, 10)) return std::make_pair(-1, data());
        bool need_padding = (data_lenght * tobits) % tobits == 0;   
        if (!convertbits(need_padding, frombits, tobits,conv, data(dec.data.begin() + data_part_pointer + 3, dec.data.begin() + data_part_pointer + 3 + data_lenght))) {
            return std::make_pair(-1, data());
        }
        switch(dec.data[data_part_pointer]){
            case 1:                             // 'p' Preimage of this provides proof of payment.
                preimage = conv;
                break;
            case 16:                            // 's' This 256-bit secret prevents forwarding nodes from probing the payment recipient.
               secret = conv;
                break;
            case 'd':                           // Short description of purpose of payment (UTF-8)
                break;
            case 'm':                           // Additional metadata to attach to the payment.
                break;
            case 'h':                           // 256-bit description of purpose of payment (SHA256).
                break;
            case 'x':                           // expiry time in seconds (big-endian). Default is 3600 (1 hour) if not specified.
                break;
            case 'c':                           // min_final_cltv_expiry_delta to use for the last HTLC in the route. Default is 18 if not specified.
                break;
            case 'f':                           // variable, depending on version. Fallback on-chain address: for Bitcoin, this starts with a 5-bit version and contains a witness program or P2PKH or P2SH address.
                break;
            case 'r':                           // One or more entries containing extra routing information for a private route; there may be more than one r field
                break;
            case '9':
                break;
            default:                            // MUST skip over unknown fields
                break;
        }

    } while (true);
 

}

}
