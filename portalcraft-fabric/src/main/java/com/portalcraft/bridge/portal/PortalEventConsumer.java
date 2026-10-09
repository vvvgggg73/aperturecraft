package com.portalcraft.bridge.portal;

import com.portalcraft.bridge.PortalCraftClientMod;
import com.portalcraft.bridge.ipc.BridgeLayout;
import com.portalcraft.bridge.ipc.WindowsSharedMemory;

import java.nio.ByteBuffer;

public final class PortalEventConsumer {
    private final WindowsSharedMemory sharedMemory;

    public PortalEventConsumer(WindowsSharedMemory sharedMemory) {
        this.sharedMemory = sharedMemory;
    }

    public void pump() {
        ByteBuffer buffer = sharedMemory.buffer();
        int readIndex = buffer.getInt(BridgeLayout.EVENT_READ_INDEX_OFFSET);
        int writeIndex = buffer.getInt(BridgeLayout.EVENT_WRITE_INDEX_OFFSET);

        while (readIndex < writeIndex) {
            int slot = readIndex % BridgeLayout.EVENT_CAPACITY;
            int offset = BridgeLayout.EVENT_BUFFER_OFFSET + (slot * BridgeLayout.EVENT_SIZE);

            long sequence = buffer.getLong(offset + BridgeLayout.EVENT_SEQUENCE_OFFSET);
            int kind = buffer.getInt(offset + BridgeLayout.EVENT_KIND_OFFSET);
            double x = buffer.getDouble(offset + BridgeLayout.EVENT_POS_X_OFFSET);
            double y = buffer.getDouble(offset + BridgeLayout.EVENT_POS_Y_OFFSET);
            double z = buffer.getDouble(offset + BridgeLayout.EVENT_POS_Z_OFFSET);

            PortalCraftClientMod.LOGGER.info(
                "Received portal event seq={} kind={} at ({}, {}, {})",
                sequence,
                kind,
                x,
                y,
                z
            );

            // TODO: Replace logging with portal-anchor placement and momentum-preserving teleport logic.
            readIndex++;
            buffer.putInt(BridgeLayout.EVENT_READ_INDEX_OFFSET, readIndex);
        }
    }
}
