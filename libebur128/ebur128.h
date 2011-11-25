/* See LICENSE file for copyright and license details. */
#ifndef EBUR128_H_
#define EBUR128_H_

#ifndef EBUR128_USE_SPEEX_RESAMPLER
  /** Set this to 1 to enable true peak scanning */
  #define EBUR128_USE_SPEEX_RESAMPLER 1
#endif

/** \file ebur128.h
 *  \brief libebur128 - a library for loudness measurement according to
 *         the EBU R128 standard.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>       /* for size_t */

/** \enum channel
 *  Use these values when setting the channel map with ebur128_set_channel().
 */
enum channel {
  EBUR128_UNUSED = 0,     /**< unused channel (for example LFE channel) */
  EBUR128_LEFT,           /**< left channel */
  EBUR128_RIGHT,          /**< right channel */
  EBUR128_CENTER,         /**< center channel */
  EBUR128_LEFT_SURROUND,  /**< left surround channel */
  EBUR128_RIGHT_SURROUND, /**< right surround channel */
  EBUR128_DUAL_MONO       /**< a channel that is counted twice */
};

/** \enum error
 *  Error return values.
 */
enum error {
  EBUR128_SUCCESS = 0,
  EBUR128_ERROR_NOMEM,
  EBUR128_ERROR_INVALID_MODE,
  EBUR128_ERROR_INVALID_CHANNEL_INDEX,
  EBUR128_ERROR_NO_CHANGE
};

/** \enum mode
 *  Use these values in ebur128_init (or'ed). Try to use the lowest possible
 *  modes that suit your needs, as performance will be better.
 */
enum mode {
  /** can call ebur128_loudness_momentary */
  EBUR128_MODE_M           = (1 << 0),
  /** can call ebur128_loudness_shortterm */
  EBUR128_MODE_S           = (1 << 1) | EBUR128_MODE_M,
  /** can call ebur128_gated_loudness_* */
  EBUR128_MODE_I           = (1 << 2) | EBUR128_MODE_M,
  /** can call ebur128_loudness_range */
  EBUR128_MODE_LRA         = (1 << 3) | EBUR128_MODE_S,
  /** can call ebur128_sample_peak */
  EBUR128_MODE_SAMPLE_PEAK = (1 << 4) | EBUR128_MODE_M,
  /** uses histogram algorithm to calculate loudness */
#if EBUR128_USE_SPEEX_RESAMPLER
  /** can call ebur128_true_peak */
  EBUR128_MODE_TRUE_PEAK   = (1 << 5) | EBUR128_MODE_M,
#endif
  EBUR128_MODE_HISTOGRAM   = (1 << 6)
};

/** forward declaration of ebur128_state_internal */
struct ebur128_state_internal;

/** \brief Contains information about the state of a loudness measurement.
 *
 *  You should not need to modify this struct directly.
 */
typedef struct {
  int mode;                           /**< The current mode. */
  unsigned int channels;              /**< The number of channels. */
  unsigned long samplerate;           /**< The sample rate. */
  struct ebur128_state_internal* d;   /**< Internal state. */
} ebur128_state;

/** \brief Initialize library state.
 *
 *  @param channels the number of channels.
 *  @param samplerate the sample rate.
 *  @param mode see the mode enum for possible values.
 *  @return an initialized library state.
 */
ebur128_state* ebur128_init(unsigned int channels,
                            unsigned long samplerate,
                            int mode);

/** \brief Destroy library state.
 *
 *  @param st pointer to a library state.
 */
void ebur128_destroy(ebur128_state** st);

/** \brief Set channel type.
 *
 *  The default is:
 *  - 0 -> EBUR128_LEFT
 *  - 1 -> EBUR128_RIGHT
 *  - 2 -> EBUR128_CENTER
 *  - 3 -> EBUR128_UNUSED
 *  - 4 -> EBUR128_LEFT_SURROUND
 *  - 5 -> EBUR128_RIGHT_SURROUND
 *
 *  @param st library state.
 *  @param channel_number zero based channel index.
 *  @param value channel type from the "channel" enum.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_CHANNEL_INDEX if invalid channel index.
 */
int ebur128_set_channel(ebur128_state* st,
                        unsigned int channel_number,
                        int value);

/** \brief Change library parameters.
 *
 *  Note that the channel map will be reset when setting a different number of
 *  channels. The current unfinished block will be lost.
 *
 *  @param st library state.
 *  @param channels new number of channels.
 *  @param samplerate new sample rate.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_NOMEM on memory allocation error. The state will be
 *      invalid and must be destroyed.
 *    - EBUR128_ERROR_NO_CHANGE if channels and sample rate were not changed.
 */
int ebur128_change_parameters(ebur128_state* st,
                              unsigned int channels,
                              unsigned long samplerate);

/** \brief Add frames to be processed.
 *
 *  @param st library state.
 *  @param src array of source frames. Channels must be interleaved.
 *  @param frames number of frames. Not number of samples!
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_NOMEM on memory allocation error.
 */
int ebur128_add_frames_short(ebur128_state* st,
                             const short* src,
                             size_t frames);
/** \brief See \ref ebur128_add_frames_short */
int ebur128_add_frames_int(ebur128_state* st,
                             const int* src,
                             size_t frames);
/** \brief See \ref ebur128_add_frames_short */
int ebur128_add_frames_float(ebur128_state* st,
                             const float* src,
                             size_t frames);
/** \brief See \ref ebur128_add_frames_short */
int ebur128_add_frames_double(ebur128_state* st,
                             const double* src,
                             size_t frames);

/** \brief Get global integrated loudness in LUFS.
 *
 *  @param st library state.
 *  @param out integrated loudness in LUFS. -HUGE_VAL if result is negative
 *             infinity.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_I" has not been set.
 */
int ebur128_loudness_global(ebur128_state* st, double* out);
/** \brief Get global integrated loudness in LUFS across multiple instances.
 *
 *  @param sts array of library states.
 *  @param size length of sts
 *  @param out integrated loudness in LUFS. -HUGE_VAL if result is negative
 *             infinity.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_I" has not been set.
 */
int ebur128_loudness_global_multiple(ebur128_state** sts,
                                     size_t size,
                                     double* out);

/** \brief Get momentary loudness (last 400ms) in LUFS.
 *
 *  @param st library state.
 *  @param out momentary loudness in LUFS. -HUGE_VAL if result is negative
 *             infinity.
 *  @return
 *    - EBUR128_SUCCESS on success.
 */
int ebur128_loudness_momentary(ebur128_state* st, double* out);
/** \brief Get short-term loudness (last 3s) in LUFS.
 *
 *  @param st library state.
 *  @param out short-term loudness in LUFS. -HUGE_VAL if result is negative
 *             infinity.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_S" has not been set.
 */
int ebur128_loudness_shortterm(ebur128_state* st, double* out);

/** \brief Get loudness range (LRA) of programme in LU.
 *
 *  Calculates loudness range according to EBU 3342.
 *
 *  @param st library state.
 *  @param out loudness range (LRA) in LU. Will not be changed in case of
 *             error. EBUR128_ERROR_NOMEM or EBUR128_ERROR_INVALID_MODE will be
 *             returned in this case.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_NOMEM in case of memory allocation error.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_LRA" has not been set.
 */
int ebur128_loudness_range(ebur128_state* st, double* out);
/** \brief Get loudness range (LRA) in LU across multiple instances.
 *
 *  Calculates loudness range according to EBU 3342.
 *
 *  @param sts array of library states.
 *  @param size length of sts
 *  @param out loudness range (LRA) in LU. Will not be changed in case of
 *             error. EBUR128_ERROR_NOMEM or EBUR128_ERROR_INVALID_MODE will be
 *             returned in this case.
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_NOMEM in case of memory allocation error.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_LRA" has not been set.
 */
int ebur128_loudness_range_multiple(ebur128_state** sts,
                                    size_t size,
                                    double* out);

/** \brief Get maximum sample peak of selected channel in float format.
 *
 *  @param st library state
 *  @param channel_number channel to analyse
 *  @param out maximum sample peak in float format (1.0 is 0 dBFS)
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_SAMPLE_PEAK" has not
 *      been set.
 *    - EBUR128_ERROR_INVALID_CHANNEL_INDEX if invalid channel index.
 */
int ebur128_sample_peak(ebur128_state* st,
                        unsigned int channel_number,
                        double* out);
#if EBUR128_USE_SPEEX_RESAMPLER
/** \brief Get maximum true peak of selected channel in float format.
 *
 *  Uses the Speex resampler with quality level 8 to calculate true peak. Will
 *  oversample 4x for sample rates < 96000 Hz, 2x for sample rates < 192000 Hz
 *  and leave the signal unchanged for 192000 Hz.
 *
 *  @param st library state
 *  @param channel_number channel to analyse
 *  @param out maximum true peak in float format (1.0 is 0 dBFS)
 *  @return
 *    - EBUR128_SUCCESS on success.
 *    - EBUR128_ERROR_INVALID_MODE if mode "EBUR128_MODE_TRUE_PEAK" has not
 *      been set.
 *    - EBUR128_ERROR_INVALID_CHANNEL_INDEX if invalid channel index.
 */
int ebur128_true_peak(ebur128_state* st,
                      unsigned int channel_number,
                      double* out);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* EBUR128_H_ */
