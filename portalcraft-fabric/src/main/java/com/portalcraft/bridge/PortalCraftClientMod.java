package com.portalcraft.bridge;

import com.portalcraft.bridge.ipc.WindowsSharedMemory;
import com.portalcraft.bridge.portal.PortalEventConsumer;
import com.portalcraft.bridge.sync.PlayerSyncPublisher;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class PortalCraftClientMod implements ClientModInitializer {
    public static final String MOD_ID = "portalcraft_bridge";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    private static WindowsSharedMemory sharedMemory;
    private static PlayerSyncPublisher syncPublisher;
    private static PortalEventConsumer portalEventConsumer;

    @Override
    public void onInitializeClient() {
        sharedMemory = WindowsSharedMemory.openOrCreate();
        syncPublisher = new PlayerSyncPublisher(sharedMemory);
        portalEventConsumer = new PortalEventConsumer(sharedMemory);
        ClientTickEvents.END_CLIENT_TICK.register(client -> portalEventConsumer.pump());
        LOGGER.info("PortalCraft bridge initialized with mapping {}", WindowsSharedMemory.MAPPING_NAME);
    }

    public static PlayerSyncPublisher getSyncPublisher() {
        return syncPublisher;
    }

    public static WindowsSharedMemory getSharedMemory() {
        return sharedMemory;
    }
}
