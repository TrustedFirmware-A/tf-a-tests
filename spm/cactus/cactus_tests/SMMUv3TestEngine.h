/*
 * Copyright (c) 2015-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* -*- C -*-
 *
 * Copyright 2015 ARM Limited. All rights reserved.
 */

#ifndef ARM_INCLUDE_SMMUv3TestEngine_h
#define ARM_INCLUDE_SMMUv3TestEngine_h

#include <inttypes.h>

///
/// Notes on interfacing to PCIe
/// ----------------------------
///
/// MSIAddress and MSIData are held in the MSI Table that is found by a BAR.
///
/// So if operating under PCIe then MSIAddress should be '1' and MSIData is
/// interpreted as the vector to use (0..2048).  If MSIAddress is not '0' or '1'
/// then the frame is misconfigured.
///
/// StreamID is not run-time assignable as it is an attribute of the topology of
/// the system.
///
/// In PCIe, then we need multiple instances of the engine and it shall occupy
/// one Function.
///
/// Each BAR is 64 bits so the three BARs are:
///    * BAR0 is going to point to a set of register frames, at least 128 KiB
///    * BAR1/2 are MSI-X vector/pending bit array (PBA).
///


///
/// The engine consists of a series of contiguous pairs of 64 KiB pages, each
/// page consists of a series of frames.  The frames in the first page (User
/// Page) are expected to be able to be exposed to a low privileged piece of SW,
/// whilst the second page (Privileged Page) is expected to be controlled by a
/// higher level of SW.
///
/// Examples:
///    1) User Page       controlled by EL1
///       Privileged Page controlled by EL2
///    2) User Page       controlled by EL0
///       Privileged Page controlled by EL1
///
/// The engine can have an unlimited number of pairs.
///
/// Each pair of pages are full of register frames.  The frames are the same
/// size in both and frame N in the User page corresponds to frame N in the
/// Privileged page.
///
/// The work load is setup by filling out all the non-cmd fields and then
/// writing to cmd the command code.  If Device-nGnR(n)E is used then no
/// explicit barrier instruction is required.
///
/// When the work has finished then the engine sets cmd to ENGINE_HALTED or
/// ENGINE_ERROR depending on if the engine encountered an error.
///
/// If the command was run then an MSI will be generated if msiaddress != 0,
/// independent of if there was an error or not.  If the MSI abort then
/// uctrl.MSI_ABORTED is set.
///
/// If the frame/command was invalid for some reason then no MSI will be
/// generated under the assumption that it can't trust the msiaddress field and
/// ENGINE_FRAME_MISCONFIGURED is read out of cmd.  Thus the user should write
/// the command and then immediately read to see if it is in the
/// ENGINE_FRAME_MISCONFIGURED state.  It is guaranteed that that a read of cmd
/// after writing cmd will immediately return ENGINE_FRAME_MISCONFIGURED if the
/// command was invalid.
///
/// If the engine is not in the ENGINE_HALTED, ENGINE_ERROR or
/// ENGINE_FRAME_MISCONFIGURED state then any writes are ignored.
///
/// As this is a model-only device then the error diagnostics are crude as it is
/// expected that a verbose error trace stream will come from the model!
///
/// Most of the work-loads can be seeded to do work in a random order with
/// random transaction sizes.  The exact specification of the order and
/// transaction size are TBD.  It is intended that the algorithm used is
/// specified so that you can work out the order that it should be done in.
///
/// The device can issue multiple outstanding transactions for each work-load.
///
/// The device will accept any size access for all fields except for cmd.
///
/// If a single burst access crosses the boundary of a user_frame the result is
/// UNPREDICTABLE.  From a programmer's perspective, then you can use any way of
/// writing to within the same frame.  However, you should only write to cmd_
/// separately with a single 32 bit access.
///
/// Whilst running the whole frame is write-ignored and the unspecified values
/// of udata and pdata are UNKNOWN.
///
/// The begin, end_incl, stride and seed are interpreted as follows:
///
///    * if [begin & ~7ull, end_incl | 7ull] == [0, ~0ull], ENGINE_FRAME_MISCONFIGURED
///       * such a huge range is not supported for any stride!
///    * stride == 0, ENGINE_FRAME_MISCONFIGURED
///    * stride == 1, then the range operated on is [begin, end_incl]
///    * stride is a multiple of 8
///       * single 64 bit transfers are performed
///       * the addresses used are:
///            (begin & ~7ull) + n * stride for n = 0..N
///         where the last byte accessed is <= (end_incl | 7)
///    * for any other value of stride, ENGINE_FRAME_MISCONFIGURED
///    * if stride > max(8, end_incl - begin + 1) then only a single
///      element is transferred.
///    * seed == 0 then the sequence of operation is n = 0, 1, 2, .. N
///      though multiple in flight transactions could alter this order.
///    * seed == ~0u then the sequence is n = N, N-1, N-2, .. 0
///    * seed anything else then sequence randomly pulls one off the front
///      or the back of the range.
///
/// The random number generator R is defined as:
inline uint32_t testengine_random(uint64_t* storage_)
{
    *storage_ = (
        *storage_ * 0x0005deecE66Dull + 0xB
        ) & 0xffffFFFFffffull;
    uint32_t const t = uint32_t((*storage_ >> 17 /* NOTE */) & 0x7FFFffff);

    //
    // Construct the topmost bit by running the generator again and
    // choosing a bit from somewhere
    //
    *storage_ = (
        *storage_ * 0x0005deecE66Dull + 0xB
        ) & 0xffffFFFFffffull;
    uint32_t const ret = uint32_t(t | (*storage_ & 0x80000000ull));
    return ret;
}

// Seeding storage from the 'seed' field is:
inline void testengine_random_seed_storage(uint64_t* storage_, uint32_t seed_)
{
    *storage_ =  uint64_t(seed_) << 16 | 0x330e;
}


/// 128 bytes
struct user_frame_t
{
    // -- 0 --
    uint32_t     cmd;
    uint32_t     uctrl;

    // -- 1 --
    // These keep track of how much work is being done by the engine.
    uint32_t     count_of_transactions_launched;
    uint32_t     count_of_transactions_returned;

    // -- 2 --
    // If operating under PCIe then msiaddress should be either 1 (send MSI-X)
    // or 0 (don't send).  The MSI-X to send is in msidata.
    uint64_t     msiaddress;

    // -- 3 --
    // If operating under PCIe then msidata is the MSI-X index in the MSI-X
    // vector table to send (0..2047)
    //
    // If operating under PCIe then msiattr has no effect.
    uint32_t     msidata;
    uint32_t     msiattr; // encoded same bottom half of attributes field

    //
    // source and destination attributes, including NS attributes if SSD-s
    // Includes 'instruction' attributes so the work load can look like
    // instruction accesses.
    //
    // Each halfword encodes:
    //     15:14  shareability 0..2 (nsh/ish/osh) (ACE encoding), ignored if a device type
    //     13     outer transient, ignored unless outer ACACHE is cacheable
    //     12     inner transient, ignored unless inner ACACHE is cacheable
    //     10:8   APROT (AMBA encoding)
    //            10 InD -- Instruction not Data
    //             9 NS  -- Non-secure
    //             8 PnU -- Privileged not User
    //     7:4    ACACHE encoding of outer
    //     3:0    if 7:4 == {0,1}
    //                // Device type
    //                3    Gathering if ACACHE is 1, ignored otherwise
    //                2    Reordering if ACACHE is 1, ignored otherwise
    //            else
    //                // Normal type
    //                ACACHE encoding of inner
    //
    // ACACHE encodings:
    //      0000 -- Device-nGnRnE
    //      0001 -- Device-(n)G(n)RE -- depending on bits [3:2]
    //      0010 -- NC-NB (normal non-cacheable non-bufferable)
    //      0011 -- NC
    //      0100 -- illegal
    //      0101 -- illegal
    //      0110 -- raWT
    //      0111 -- raWB
    //      1000 -- illegal
    //      1001 -- illegal
    //      1010 -- waWT
    //      1011 -- waWB
    //      1100 -- illegal
    //      1101 -- illegal
    //      1110 -- rawaWT
    //      1111 -- rawaWB
    //
    // NOTE that the meaning of the ACACHE encodings are dependent on if it is a
    // read or a write.  AMBA can't encode directly the 'no-allocate cacheable'
    // and you have to set the 'other' allocation hint.  So for example, a read
    // naWB has to be encoded as waWB.  A write naWB has to be encoded as raWB,
    // etc.
    //
    // Lowest halfword are 'source' attributes.
    // Highest halfword are 'destination' attributes.
    //
    // NOTE that you can make an non-secure stream output a secure transaction
    // -- the SMMU should sort it out.
    //

    // -- 4 --
    // Under PCIe then a real Function does not have control over the attributes
    // of the transactions that it makes.  However, for testing purposes of the
    // SMMU then we allow its attributes to be specified (and magically
    // transport them over PCIe).
    uint32_t     attributes;
    uint32_t     seed;

    // -- 5 --
    uint64_t     begin;
    // -- 6 --
    uint64_t     end_incl;

    // -- 7 --
    uint64_t     stride;

    // -- 8 --
    uint64_t     udata[8];
};

// 128 bytes
struct privileged_frame_t
{
    // -- 0 --
    uint32_t     pctrl;
    uint32_t     downstream_port_index; // [0,64), under PCIe only use port 0

    // -- 1 --
    // Under PCIe, then streamid is ignored.
    uint32_t     streamid;
    uint32_t     substreamid; // ~0u means no substreamid, otherwise must be a 20 bit number or ENGINE_FRAME_MISCONFIGURED

    // -- 2 --
    uint64_t     pdata[14];
};

// 128 KiB
struct engine_pair_t
{
    user_frame_t         user[ 64 * 1024 / sizeof(user_frame_t)];
    privileged_frame_t   privileged[ 64 * 1024 / sizeof(privileged_frame_t)];
};

//
// NOTE that we don't have a command that does some writes then some reads.  For
// the ACK this is probably not going to be much of a problem.
//
// On completion, an MSI will be sent if the msiaddress != 0.
//
enum cmd_t
{
    // ORDER IS IMPORTANT, see predicates later in this file.

    // The frame was misconfigured.
    ENGINE_FRAME_MISCONFIGURED = ~0u - 1,

    // The engine encountered an error (downstream transaction aborted).
    ENGINE_ERROR  = ~0u,

    // This frame is unimplemented or in use by the secure world.
    //
    // A user _can_ write this to cmd and it will be considered to be
    // ENGINE_HALTED.
    ENGINE_NO_FRAME = 0,

    // The engine is halted.
    ENGINE_HALTED = 1,

    // The engine memcpy's from region [begin, end_incl] to address udata[0].
    //
    // If stride is 0 then ENGINE_ERROR is produced, udata[2] contains the error
    // address.  No MSI is generated.
    //
    // If stride is 1 then this is a normal memcpy().  If stride is larger then
    // not all the data will be copied.
    //
    // The order and size of the transactions used are determined randomly using
    // seed.  If seed is:
    //     0  -- do them from lowest address to highest address
    //    ~0u -- do them in reverse order
    //    otherwise use the value as a seed to do them in random order
    // The ability to do them in a  non-random order means that we stand a
    // chance of getting merged event records.
    //
    // This models a work-load where we start with some reads and then do some
    // writes.
    ENGINE_MEMCPY = 2,

    // The engine randomizes region [begin, end_incl] using rand48, seeded
    // with seed and using the specified stride.
    //
    // The order and size of the transactions used are determined randomly using
    // seed.
    //
    // The seed is used to create a random number generator that is used to
    // choose the direction.
    //
    // A separate random number generator per transaction is then used based on
    // seed and the address:
    //
    //       seed_per_transaction = seed ^ (address >> 32) ^ (address & 0xFFFFffff);
    //
    // This seed is then used to seed a random number generator to fill the
    // required space.   The data used should be:
    //          uint64_t storage;
    //          for (uint8_t* p = (uintptr_t)begin; p != (uintptr_t)end_incl; ++ p)
    //          {
    //             // When we cross a 4 KiB we reseed.
    //             if ((p & 0xFFF) == 0 || p == begin)
    //             {
    //                  testengine_random_seed_storage(
    //                      V ^ ((uintptr_t)p >> 32) ^ (uint32_t((uintptr_t)p))
    //                      );
    //             }
    //             assert( *p == (uint8_t)testengine_random(&storage) );
    //             ++ p;
    //          }
    // This isn't the most efficient way of doing it as it throws away a lot of
    // entropy from the call to testengine_random() but then we aren't aiming for
    // good random numbers.
    //
    // If stride is 0 then ENGINE_ERROR is produced, data[2] contains the error
    // address. (NOTE that data[1] is not used).
    //
    // If stride is 1 then this fills the entire buffer.  If stride is larger
    // then not all the data will be randomized.
    //
    // This models a write-only work-load.
    ENGINE_RAND48 = 3,

    // The engine reads [begin, end_incl], treats the region as a set of
    // uint64_t and sums them, delivering the result to udata[1], using the
    // specified stride.
    //
    // If stride is 0 then ENGINE_ERROR is produced, udata[2] is the error
    // address.
    //
    // If stride is 1 then this sums the entire buffer.  If stride is larger
    // then not all the data will be summed.
    //
    // The order and size of the transactions used are determined randomly using
    // seed.
    //
    // The begin must be 64 bit aligned (begin & 7) == 0 and the end_incl must
    // end at the end of a 64 bit quantitity (end_incl & 7) == 7, otherwise
    // ENGINE_FRAME_MISCONFIGURED is generated.
    //
    // This models a read-only work-load.
    ENGINE_SUM64 = 4
};

static inline bool is_valid_and_running(cmd_t t_)
{
    unsigned const t = t_; // compensate for bad MSVC treating t_ as signed!
    return ENGINE_MEMCPY <= t && t <= ENGINE_SUM64;
}

static inline bool is_in_error_state(cmd_t t_)
{
    return t_ == ENGINE_ERROR || t_ == ENGINE_FRAME_MISCONFIGURED;
}

static inline bool is_in_error_or_stopped_state(cmd_t t_)
{
    return t_ == ENGINE_NO_FRAME
        || t_ == ENGINE_HALTED
        || is_in_error_state(t_);
}

static inline bool is_invalid(cmd_t t_)
{
    unsigned const t = t_; // compensate for bad MSVC treating t_ as signed!
    return ENGINE_SUM64 < t && t < ENGINE_FRAME_MISCONFIGURED;
}

/// pctrl has layout
///
///       0 -- SSD_NS     -- the stream and frame is non-secure
///                       -- note that if this is zero then it means the
///                          frame is controlled by secure SW and non-secure
///                          accesses are RAZ/WI (and so see ENGINE_NO_FRAME)
///                          Secure SW can only generate secure SSD StreamIDs
///                          This could be relaxed in the future if people need
///                          to.
///
///       8 -- ATS_ENABLE -- CURRENTLY HAS NO EFFECT
///       9 -- PRI_ENABLE -- CURRENTLY HAS NO EFFECT
///
/// SSD_NS can only be altered by a secure access.  Once clear then the
/// corresponding user and privileged frames are accessible only to secure
/// accesses.  Non-secure accesses are RAZ/WI (and hence cmd will be
/// ENGINE_NO_FRAME to non-secure accesses).
///
/// ATS_ENABLE/PRI_ENABLE are not currently implemented and their intent is for
/// per-substreamid ATS/PRI support.
///
/// However, ATS/PRI support for the whole StreamID is advertised through the
/// PCIe Extended Capabilities Header.
///

/// uctrl has layout
///
///       0     -- MSI_ABORTED -- an MSI aborted (set by the engine)
///
///       16-31 -- RATE   -- some ill-defined metric for how fast to do the work!
///

#endif
