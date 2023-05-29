/*
 * voicerec.h
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef VOICEREC_H
#define VOICEREC_H

extern void voicerec_storage_init(void);
extern void voicerec_storage_cleanup(void);
extern void voicerec_wav_publish(const char *filename);
extern void wav_start(void);
extern void wav_pcm_in(const void *pcm, size_t size);
extern void wav_stop(void);
extern const char *wav_filename(void);
extern unsigned int wav_time_elapsed_ms(void);
extern void filter_pcm_in(const void *pcm, size_t size);

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
