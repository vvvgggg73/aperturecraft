package com.portalcraft.bridge.ipc;

import com.portalcraft.bridge.PortalCraftClientMod;
import com.sun.jna.Pointer;
import com.sun.jna.platform.win32.Kernel32;
import com.sun.jna.platform.win32.WinBase;
import com.sun.jna.platform.win32.WinNT;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public final class WindowsSharedMemory {
    public static final String MAPPING_NAME = "Local\\PortalCraftBridge";
    private static final int FILE_MAP_ALL_ACCESS = 0x000F001F;
    private static final int PAGE_READWRITE = 0x04;

    private final WinNT.HANDLE mappingHandle;
    private final Pointer basePointer;
    private final ByteBuffer buffer;

    private WindowsSharedMemory(WinNT.HANDLE mappingHandle, Pointer basePointer) {
        this.mappingHandle = mappingHandle;
        this.basePointer = basePointer;
        this.buffer = basePointer.getByteBuffer(0, BridgeLayout.TOTAL_SIZE).order(ByteOrder.LITTLE_ENDIAN);
        initializeHeaderIfNeeded();
    }

    public static WindowsSharedMemory openOrCreate() {
        Kernel32 kernel32 = Kernel32.INSTANCE;
        WinNT.HANDLE handle = kernel32.OpenFileMapping(FILE_MAP_ALL_ACCESS, false, MAPPING_NAME);

        if (handle == null) {
            handle = kernel32.CreateFileMapping(
                WinBase.INVALID_HANDLE_VALUE,
                null,
                PAGE_READWRITE,
                0,
                BridgeLayout.TOTAL_SIZE,
                MAPPING_NAME
            );
        }

        if (handle == null) {
            throw new IllegalStateException("Unable to open shared memory mapping " + MAPPING_NAME);
        }

        Pointer pointer = kernel32.MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, BridgeLayout.TOTAL_SIZE);
        if (pointer == null) {
            kernel32.CloseHandle(handle);
            throw new IllegalStateException("Unable to map view for " + MAPPING_NAME);
        }

        return new WindowsSharedMemory(handle, pointer);
    }

    private void initializeHeaderIfNeeded() {
        if (buffer.getInt(BridgeLayout.MAGIC_OFFSET) != BridgeLayout.MAGIC) {
            for (int i = 0; i < BridgeLayout.TOTAL_SIZE; i++) {
                buffer.put(i, (byte) 0);
            }

            buffer.putInt(BridgeLayout.MAGIC_OFFSET, BridgeLayout.MAGIC);
            buffer.putInt(BridgeLayout.VERSION_OFFSET, BridgeLayout.VERSION);
        }
    }

    public void writeMinecraftPlayerState(
        double x,
        double y,
        double z,
        double velocityX,
        double velocityY,
        double velocityZ,
        float yaw,
        float pitch,
        long frameId
    ) {
        writePlayerState(
            BridgeLayout.MINECRAFT_STATE_OFFSET,
            x,
            y,
            z,
            velocityX,
            velocityY,
            velocityZ,
            yaw,
            pitch,
            frameId
        );
        buffer.putInt(BridgeLayout.HOST_HEARTBEAT_OFFSET, (int) (System.nanoTime() & 0x7FFFFFFF));
    }

    public void writePlayerState(
        int baseOffset,
        double x,
        double y,
        double z,
        double velocityX,
        double velocityY,
        double velocityZ,
        float yaw,
        float pitch,
        long frameId
    ) {
        buffer.putDouble(baseOffset + BridgeLayout.POS_X_OFFSET, x);
        buffer.putDouble(baseOffset + BridgeLayout.POS_Y_OFFSET, y);
        buffer.putDouble(baseOffset + BridgeLayout.POS_Z_OFFSET, z);
        buffer.putDouble(baseOffset + BridgeLayout.VEL_X_OFFSET, velocityX);
        buffer.putDouble(baseOffset + BridgeLayout.VEL_Y_OFFSET, velocityY);
        buffer.putDouble(baseOffset + BridgeLayout.VEL_Z_OFFSET, velocityZ);
        buffer.putFloat(baseOffset + BridgeLayout.YAW_OFFSET, yaw);
        buffer.putFloat(baseOffset + BridgeLayout.PITCH_OFFSET, pitch);
        buffer.putLong(baseOffset + BridgeLayout.FRAME_ID_OFFSET, frameId);
    }

    public ByteBuffer buffer() {
        return buffer;
    }

    public void close() {
        Kernel32.INSTANCE.UnmapViewOfFile(basePointer);
        Kernel32.INSTANCE.CloseHandle(mappingHandle);
        PortalCraftClientMod.LOGGER.info("PortalCraft bridge mapping closed");
    }
}
