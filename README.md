## usage and testing

to test, run:

`make`

`python3 test.py ./alice_100_50.bin `

## documentation

the timestamp event is encoded in a 64-bit word. we can specify the number of bits `clock_bitwidth` for the clock value and `detector_bitwidth` for the detector bitmask. the remaining bits in between are left uninitialised as random values.

the encoder and decoder agree beforehand on the parameters `clock_bitwidth` and `detector_bitwidth`. in the following illustration, we initialise:
- `clock_bitwidth` = 49 bits;
- `detector_bitwidth` = 4 bits.

![](https://i.imgur.com/J7v4AT6.png)

## clock value compression
here we consider a differential compression protocol for clock values:

- except for the initial events, clock values are not recorded in full; rather, we only store the differences `t_diff` between one value and the next, and recover the full timeseries later by adding them up. 
- we encode these `t_diff` as bitstrings. since `t_diff` varies across events, the number of bits needed to encode them `t_diff_bitwidth` also varies. to save space, we dynamically allocate bits according to whether `t_diff_bitwidth` is increasing or decreasing. 
- we expect `t_diff_bitwidth` to eventually stabilise.

the subtlety here is that the decoder has to calculate the varying `t_diff_bitwidth` using the same strategy as the encoder. otherwise, she will be reading her incoming stream in wrongly-sized chunks, and recovering gibberish instead of the correct `t_diff`s.

here, we describe how the decoder can deterministically derive the correct bitwidths used by the encoder:

### 0. initial `t_diff_bitwidth`

![](https://i.imgur.com/VHaiwnZ.png)

#### encoder
- initialise `t_diff_bitwidth` = `clock_bitwidth` = 49 bits;
- calculate `t_diff = t_1 - t_0`;
- `encode(t_diff)` in a 49-bit word by left-padding it with `0`s;
- send to decoder: 
    - `t_0` (49 bits)
    - `encode(t_diff)` (49 bits)
- check if `length(t_diff)` < `t_diff_bitwidth` - 1, i.e. if `t_diff` could have been encoded using one less bit. if yes, update `t_diff_bitwidth` = `t_diff_bitwidth` - 1;
    -  in our case, we update `t_diff_bitwidth` = (49 - 1) bits = 48 bits.

#### decoder
for the first events, the decoder simply reads her stream of incoming bits in 49-bit chunks (as agreed `clock_bitwidth`):
- read the first 49 bits as `t_0`;
- read the next 49 bits as `t_diff`;
- recover `t_1` = `t_0` + `t_diff`;
- check if `t_diff` < 2^49 - 1, i.e. if `t_diff` could have been encoded using one less bit. if yes, set `t_diff_bitwidth` = `clock_bitwidth` - 1.
    -  in our case, we set `t_diff_bitwidth` = (49 - 1) bits = 48 bits.

### 1. decreasing `t_diff_bitwidth`

suppose our next `t_diff` is smaller (18 bits). we accordingly decrease `t_diff_bitwidth` using a similar strategy:

![](https://i.imgur.com/f6zxXMk.png)

#### encoder
- recall that in the first step we had updated `t_diff_bitwidth` = 48 bits;
- calculate `t_diff = t_2 - t_1`;
- `encode(t_diff)` in a 48-bit word by left-padding it with `0`s;
- send to decoder: 
    - `encode(t_diff)` (48 bits)
- check if `length(t_diff)` < `t_diff_bitwidth` - 1, i.e. if `t_diff` could have been encoded using one less bit. if yes, update `t_diff_bitwidth` = `t_diff_bitwidth` - 1;
    -  in our case, we update `t_diff_bitwidth` = (48 - 1) bits = 47 bits.

#### decoder
- recall that in the first step we had set `t_diff_bitwidth` = 48 bits;
- read the next 48 bits as `t_diff`;
- recover `t_2` = `t_1` + `t_diff`;
- check if `t_diff` < 2^`t_diff_bitwidth` - 1, i.e. if `t_diff` could have been encoded using one less bit. if yes, update `t_diff_bitwidth` = `t_diff_bitwidth` - 1;
    -  in our case, we update `t_diff_bitwidth` = (48 - 1) bits = 47 bits.

### 2. increasing `t_diff_bitwidth`
suppose now that we are faced with a larger `t_diff` than our bitwidth allows, meaning we have to increase `t_diff_bitwidth`.

![](https://i.imgur.com/etCPR9X.png)

#### encoder
- suppose that by this point, our `t_diff_bitwidth` had been reduced to 20 bits (which is 2 bits too small for our the next `t_diff`);
- calculate `t_diff` = `t_9` - `t_8`;
- check if `length(t_diff)` > `t_diff_bitwidth`, i.e. whether we need more bits to represent `t_diff`;
	- if yes, calculate the new `t_diff_bitwidth` = `floor(log_2(t_diff))` + 1;
	- in our case, the new `t_diff_bitwidth` is 22 bits.
- send to decoder:
    - 20-bit `000000...` string of zeros, flagging an increase in bitwidth;
	- the bitstring `001`, where the number of leading zeros is the number of bits by which to increase bitwidth;
    - 22-bit `t_diff`.

#### decoder
- at this point our the decoder has the same starting `t_diff_bitwidth` = 20 as the encoder;
- she reads the incoming bitstring as such:
	- she expects to read `t_diff` in the next 20 bits, but instead sees `000000...` a 20-bit string of zeros. she understands this as the start of the encoding of a larger `t_diff`;
	- she reads the following bits, counting the number of `0`s `num_zeros` before the first `1`. she increases `t_diff_bitwidth` = `t_diff_bitwidth` + `num_zeros`;
	- in our case, we get `t_diff_bitwidth` = (20 + 2) bits = 22 bits;
	- she reads the next 22 bits as `t_diff`.
	
### 3. stable `t_diff_bitwidth`

we expect that `t_diff_bitwidth` will eventually stabilise.

#### encoder
as mentioned above, the encoder should perform certain checks on `t_diff` before and after encoding, in the following order:

i) check if `length(t_diff)` > `t_diff_bitwidth`, i.e. whether we need more bits to represent `t_diff`. if yes, encode using the "increasing `t_diff_bitwidth`" algorithm and increase `t_diff_bitwidth`;
ii) else, pad `t_diff` to `t_diff_bitwidth`;
iii) check if `length(t_diff)` < `t_diff_bitwidth`; if yes, decrease `t_diff_bitwidth` by one bit;
iv) else, leave `t_diff_bitwidth` unchanged.

#### decoder
the decoder does not need to check for increasing bitwidths, since that will be made clear by the chunks of `000000...` zero bitstrings, and she will be forced to increase `t_diff_bitwidth` following the algorithm. however, she should:

i) check if `t_diff` < 2^`t_diff_bitwidth` - 1; if yes, decrease `t_diff_bitwidth` by one bit;
ii) else, leave `t_diff_bitwidth` unchanged.