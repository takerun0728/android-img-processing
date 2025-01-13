package com.example.android_img_processing

import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import android.widget.Toast
import com.example.android_img_processing.databinding.ActivityMainBinding
import org.opencv.android.CameraActivity
import org.opencv.android.CameraBridgeViewBase
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2
import org.opencv.android.OpenCVLoader
import org.opencv.core.CvType
import org.opencv.core.Mat


class MainActivity : CameraActivity(), CvCameraViewListener2  {

    private lateinit var binding: ActivityMainBinding
    private lateinit var m: Mat
    private lateinit var openCvCameraView: CameraBridgeViewBase

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (OpenCVLoader.initLocal()) {
            Log.i("Android Image Processing", "OpenCV loaded successfully")
        } else {
            Log.i("Android Image Processing", "OpenCV initialization failed!")
            (Toast.makeText(this, "OpenCV initialization failed!", Toast.LENGTH_LONG)).show()
            return
        }

        System.loadLibrary("android_img_processing")
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        // Example of a call to a native method
        binding.transPixels.text = "test"
        binding.transPixels.bringToFront()
        openCvCameraView = binding.cameraView
        openCvCameraView.visibility = CameraBridgeViewBase.VISIBLE
        openCvCameraView.setCvCameraViewListener(this)
    }

    override fun onCameraViewStarted(width: Int, height: Int) {
        m = Mat(height, width, CvType.CV_8UC4)
    }

    override fun onCameraViewStopped() {
        m.release()
    }

    override fun onCameraFrame(inputFrame: CvCameraViewFrame): Mat {
        m = inputFrame.rgba()
        calcOpticalFlow(m.nativeObjAddr)
        return m
    }

    override fun onPause() {
        super.onPause()
        openCvCameraView.disableView()
    }

    override fun onResume() {
        super.onResume()
        openCvCameraView.enableView()
    }

    override fun getCameraViewList(): List<CameraBridgeViewBase> {
        return listOf<CameraBridgeViewBase>(openCvCameraView)
    }

    override fun onDestroy() {
        super.onDestroy()
        openCvCameraView.disableView()
    }

    external fun calcOpticalFlow(matAddr: Long, resultAddr: Long)
    external fun release()
}