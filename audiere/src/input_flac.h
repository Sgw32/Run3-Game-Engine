#ifndef INPUT_FLAC_H
#define INPUT_FLAC_H

#include <FLAC/stream_decoder.h>
#include "audiere.h"
#include "basic_source.h"
#include "utility.h"

namespace audiere {
class FLACInputStream : public BasicSource {
public:
  FLACInputStream();
  ~FLACInputStream();
  bool initialize(FilePtr file);
  bool initialize(const char *filename);
  void ADR_CALL getFormat(int &, int &, SampleFormat &);
  int doRead(int, void *);
  void ADR_CALL reset();
  bool ADR_CALL isSeekable();
  int ADR_CALL getLength();
  void ADR_CALL setPosition(int);
  int ADR_CALL getPosition();
private:
  FLAC__StreamDecoderWriteStatus write(const FLAC__Frame *,
                                       const FLAC__int32 *const[]);
  static FLAC__StreamDecoderReadStatus readCallback(
      const FLAC__StreamDecoder *, FLAC__byte[], size_t *, void *);
  static FLAC__StreamDecoderSeekStatus seekCallback(
      const FLAC__StreamDecoder *, FLAC__uint64, void *);
  static FLAC__StreamDecoderTellStatus tellCallback(
      const FLAC__StreamDecoder *, FLAC__uint64 *, void *);
  static FLAC__StreamDecoderLengthStatus lengthCallback(
      const FLAC__StreamDecoder *, FLAC__uint64 *, void *);
  static FLAC__bool eofCallback(const FLAC__StreamDecoder *, void *);
  static FLAC__StreamDecoderWriteStatus writeCallback(
      const FLAC__StreamDecoder *, const FLAC__Frame *,
      const FLAC__int32 *const[], void *);
  static void metadataCallback(const FLAC__StreamDecoder *,
                               const FLAC__StreamMetadata *, void *);
  static void errorCallback(const FLAC__StreamDecoder *,
                            FLAC__StreamDecoderErrorStatus, void *);
  FilePtr m_file;
  FLAC__StreamDecoder *m_decoder{};
  SizedBuffer m_multiplexer;
  QueueBuffer m_buffer;
  int m_channel_count{};
  int m_sample_rate{};
  SampleFormat m_sample_format{SF_S16};
  unsigned m_bits_per_sample{};
  int m_length{};
  int m_position{};
};
} // namespace audiere
#endif
