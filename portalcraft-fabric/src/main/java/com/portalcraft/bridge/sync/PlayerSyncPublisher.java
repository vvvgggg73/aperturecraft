package com.portalcraft.bridge.sync;

import com.portalcraft.bridge.ipc.WindowsSharedMemory;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.util.math.Vec3d;

public final class PlayerSyncPublisher {
    private final WindowsSharedMemory sharedMemory;
    private long frameCounter;

    public PlayerSyncPublisher(WindowsSharedMemory sharedMemory) {
        this.sharedMemory = sharedMemory;
    }

    public void publishCurrentFrame(MinecraftClient client) {
        ClientPlayerEntity player = client.player;
        if (player == null) {
            return;
        }

        Vec3d velocity = player.getVelocity();
        sharedMemory.writeMinecraftPlayerState(
            player.getX(),
            player.getY(),
            player.getZ(),
            velocity.x,
            velocity.y,
            velocity.z,
            player.getYaw(),
            player.getPitch(),
            frameCounter++
        );
    }
}
