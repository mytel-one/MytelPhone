pluginManagement {
    resolutionStrategy {
        // Plugin markers for androidx / google plugins are only published on
        // google() maven and the Gradle Plugin Portal, which are unreachable or
        // incomplete on some networks; resolve through the underlying modules
        // instead (mirrored by maven-public proxies).
        eachPlugin {
            if (requested.id.id == "androidx.navigation.safeargs.kotlin") {
                useModule("androidx.navigation:navigation-safe-args-gradle-plugin:${requested.version}")
            }
            if (requested.id.id == "com.google.gms.google-services") {
                useModule("com.google.gms:google-services:${requested.version}")
            }
            if (requested.id.id == "com.google.firebase.crashlytics") {
                useModule("com.google.firebase:firebase-crashlytics-gradle:${requested.version}")
            }
        }
    }
    repositories {
        maven {
            url = uri("https://mirrors.cloud.tencent.com/nexus/repository/maven-public/")
        }
        mavenCentral()
        maven {
            url = uri("https://maven.aliyun.com/repository/google")
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        maven {
            url = uri("https://maven.aliyun.com/repository/public")
        }
        google()
        gradlePluginPortal()
    }
}

plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)

    repositories {
        maven {
            url = uri("https://mirrors.cloud.tencent.com/nexus/repository/maven-public/")
        }
        mavenCentral()
        exclusiveContent {
            forRepository { mavenCentral() }
            filter { includeModule("org.bouncycastle", "bcprov-jdk18on") }
        }
        maven {
            url = uri("https://maven.aliyun.com/repository/google")
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        maven {
            url = uri("https://maven.aliyun.com/repository/public")
        }
        google()

        // Resolve PhotoView only from its publisher. A proxy can expose its
        // POM without the AAR, which prevents Gradle falling back afterwards.
        exclusiveContent {
            forRepository {
                maven { url = uri("https://jitpack.io") }
            }
            filter {
                includeGroup("com.github.chrisbanes")
            }
        }

        exclusiveContent {
            forRepository {
                ivy {
                    name = "MyTel verified Android SIP SDK"
                    url = uri("https://github.com/mytel-one/MytelPhone/releases/download")
                    patternLayout { artifact("v[revision]/MyTel-sip-sdk-[revision].[ext]") }
                    metadataSources { artifact() }
                }
            }
            filter { includeModule("one.mytel.sdk", "sip-android") }
        }

    }
}

rootProject.name = "MyTelSoftPhone"
include(":app")
