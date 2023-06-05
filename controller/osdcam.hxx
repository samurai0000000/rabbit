/*
 * osdcam.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef OSDCAM_HXX
#define OSDCAM_HXX

class OsdCam {

public:

    static void genOsdFrame(cv::Mat &osdFrame,
                            float videoFrameRate);

};

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
