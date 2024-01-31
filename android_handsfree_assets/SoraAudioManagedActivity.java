package jp.shiguredo.sora.unityaudiomanager;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import com.unity3d.player.UnityPlayerActivity;
import jp.shiguredo.sora.audiomanager.SoraAudioManager;
import jp.shiguredo.sora.audiomanager.SoraThreadUtils;

public class SoraAudioManagedActivity extends UnityPlayerActivity {

    private static final int REQUEST_CODE = 1;
    SoraAudioManager soraAudioManager;
    SoraAudioManager.OnChangeRouteObserver observer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        soraAudioManager = SoraAudioManager.create(getApplicationContext());
    }

    @Override
    protected void onDestroy() {
        stopAudioManager();
        super.onDestroy();
    }

    // パーミッションリクエストのコールバックメソッド
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch(requestCode) {
            case REQUEST_CODE: {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    startAudioManager(this.observer);
                }
            }
        }
    }

    public void startAudioManager(SoraAudioManager.OnChangeRouteObserver observer) {
        //メインスレッドでない場合はメインスレッドで再実行
        if (!SoraThreadUtils.runOnMainThread(() -> startAudioManager(observer))) {
            return;
        }

        this.observer = observer;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                    != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT}, REQUEST_CODE);
        } else {
            soraAudioManager.start(observer);
        }
    }

    public void stopAudioManager() {
        //メインスレッドでない場合はメインスレッドで再実行
        if (!SoraThreadUtils.runOnMainThread(() -> stopAudioManager())) {
            return;
        }

        if (soraAudioManager == null) {
            return;
        }
        soraAudioManager.stop();
        soraAudioManager = null;
        observer = null;
    }

    public void setHandsfree(boolean on) {
        //メインスレッドでない場合はメインスレッドで再実行
        if (!SoraThreadUtils.runOnMainThread(() -> setHandsfree(on))) {
            return;
        }

        if (soraAudioManager == null) {
            return;
        }
        soraAudioManager.setHandsfree(on);
    }

    public boolean isHandsfree() {
        if (soraAudioManager == null) {
            return false;
        }
        return soraAudioManager.isHandsfree();
    }
}
