package com.portalcraft.bridge.mixin;

import com.portalcraft.bridge.PortalCraftClientMod;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(MinecraftClient.class)
public abstract class MinecraftClientMixin {
    @Inject(method = "render", at = @At("TAIL"))
    private void portalcraft$publishPlayerFrame(boolean tick, CallbackInfo ci) {
        if (PortalCraftClientMod.getSyncPublisher() == null) {
            return;
        }

        PortalCraftClientMod.getSyncPublisher().publishCurrentFrame((MinecraftClient) (Object) this);
    }
}
