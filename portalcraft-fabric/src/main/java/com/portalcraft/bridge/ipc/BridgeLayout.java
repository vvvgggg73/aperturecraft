package com.portalcraft.bridge.ipc;

public final class BridgeLayout {
    private BridgeLayout() {
    }

    public static final int MAGIC = 0x42544350;
    public static final int VERSION = 1;
    public static final int EVENT_CAPACITY = 64;

    public static final int HEADER_OFFSET = 0;
    public static final int PLAYER_STATE_SIZE = 64;
    public static final int EVENT_SIZE = 64;

    public static final int MAGIC_OFFSET = 0;
    public static final int VERSION_OFFSET = 4;
    public static final int HOST_HEARTBEAT_OFFSET = 8;
    public static final int GUEST_HEARTBEAT_OFFSET = 12;
    public static final int EVENT_WRITE_INDEX_OFFSET = 16;
    public static final int EVENT_READ_INDEX_OFFSET = 20;
    public static final int HEADER_SIZE = 32;

    public static final int MINECRAFT_STATE_OFFSET = HEADER_SIZE;
    public static final int SOURCE_STATE_OFFSET = MINECRAFT_STATE_OFFSET + PLAYER_STATE_SIZE;
    public static final int EVENT_BUFFER_OFFSET = SOURCE_STATE_OFFSET + PLAYER_STATE_SIZE;
    public static final int TOTAL_SIZE = EVENT_BUFFER_OFFSET + (EVENT_CAPACITY * EVENT_SIZE);

    public static final int POS_X_OFFSET = 0;
    public static final int POS_Y_OFFSET = 8;
    public static final int POS_Z_OFFSET = 16;
    public static final int VEL_X_OFFSET = 24;
    public static final int VEL_Y_OFFSET = 32;
    public static final int VEL_Z_OFFSET = 40;
    public static final int YAW_OFFSET = 48;
    public static final int PITCH_OFFSET = 52;
    public static final int FRAME_ID_OFFSET = 56;

    public static final int EVENT_SEQUENCE_OFFSET = 0;
    public static final int EVENT_KIND_OFFSET = 8;
    public static final int EVENT_FACE_OFFSET = 12;
    public static final int EVENT_POS_X_OFFSET = 16;
    public static final int EVENT_POS_Y_OFFSET = 24;
    public static final int EVENT_POS_Z_OFFSET = 32;
    public static final int EVENT_NORMAL_X_OFFSET = 40;
    public static final int EVENT_NORMAL_Y_OFFSET = 44;
    public static final int EVENT_NORMAL_Z_OFFSET = 48;
}
