#include "resources/TextureDataManager.h"

#include "resources/TextureData.h"
#include "resources/TextureResource.h"
#include "Settings.h"

TextureDataManager::TextureDataManager()
{
	unsigned char data[5 * 5 * 4];
	mBlank = std::shared_ptr<TextureData>(new TextureData(false));
	for (int i = 0; i < (5 * 5); ++i)
	{
		data[i*4] = (i % 2) * 255;
		data[i*4+1] = (i % 2) * 255;
		data[i*4+2] = (i % 2) * 255;
		data[i*4+3] = 0;
	}
	mBlank->initFromRGBA(data, 5, 5);
	mLoader = new TextureLoader;
}

TextureDataManager::~TextureDataManager()
{
	delete mLoader;
}

std::shared_ptr<TextureData> TextureDataManager::add(const TextureResource* key, bool tiled)
{
	remove(key);
	std::shared_ptr<TextureData> data(new TextureData(tiled));
	mTextures.push_front(data);
	mTextureLookup[key] = mTextures.cbegin();
	return data;
}

void TextureDataManager::remove(const TextureResource* key)
{
	// Find the entry in the list
	auto it = mTextureLookup.find(key);
	if (it != mTextureLookup.cend())
	{
		// Remove the list entry
		mTextures.erase((*it).second);
		// And the lookup
		mTextureLookup.erase(it);
	}
}

std::shared_ptr<TextureData> TextureDataManager::get(const TextureResource* key, bool enableLoading)
{
	// If it's in the cache then we want to remove it from it's current location and
	// move it to the top
	std::shared_ptr<TextureData> tex;
	auto it = mTextureLookup.find(key);
	if (it != mTextureLookup.cend())
	{
		tex = *(*it).second;
		// Remove the list entry
		mTextures.erase((*it).second);
		// Put it at the top
		mTextures.push_front(tex);
		// Store it back in the lookup
		mTextureLookup[key] = mTextures.cbegin();

		// Make sure it's loaded or queued for loading
		if (enableLoading && !tex->isLoaded())
			load(tex);
	}
	return tex;
}

bool TextureDataManager::bind(const TextureResource* key)
{
	std::shared_ptr<TextureData> tex = get(key);
	bool bound = false;
	if (tex != nullptr)
		bound = tex->uploadAndBind();
	if (!bound)
		mBlank->uploadAndBind();
	return bound;
}

size_t TextureDataManager::getTotalSize()
{
	size_t total = 0;
	for (auto tex : mTextures)
		total += tex->width() * tex->height() * 4;
	return total;
}

size_t TextureDataManager::getCommittedSize()
{
	size_t total = 0;
	for (auto tex : mTextures)
		total += tex->getVRAMUsage();
	return total;
}

size_t TextureDataManager::getQueueSize()
{
	return mLoader->getQueueSize();
}

void TextureDataManager::load(std::shared_ptr<TextureData> tex, bool block)
{
	// See if it's already loaded
	if (tex->isLoaded())
		return;
	// Not loaded. Make sure there is room
	size_t max_texture = (size_t)Settings::getInstance()->getInt("MaxVRAM") * 1024 * 1024;

	// if max_texture is 0, then texture memory should be considered unlimited
	if (max_texture > 0)
	{
		size_t size = TextureResource::getTotalMemUsage();
		for (auto it = mTextures.crbegin(); it != mTextures.crend(); ++it)
		{
			if (size < max_texture)
				break;
			//size -= (*it)->getVRAMUsage();
			(*it)->releaseVRAM();
			(*it)->releaseRAM();
			// It may be already in the loader queue. In this case it wouldn't have been using
			// any VRAM yet but it will be. Remove it from the loader queue
			mLoader->remove(*it);
			size = TextureResource::getTotalMemUsage();
		}
	}
	if (!block)
		mLoader->load(tex);
	else
		tex->load();
}

TextureLoader::TextureLoader() : mExit(false)
{
	mThread = new std::thread(&TextureLoader::threadProc, this);
}

TextureLoader::~TextureLoader()
{
	// Just abort any waiting texture
	mTextureDataQ.clear();
	mTextureDataLookup.clear();

	// Exit the thread
	mExit = true;
	mEvent.notify_one();
	mThread->join();
	delete mThread;
}

void TextureLoader::threadProc()
{
	while (!mExit)
	{
		std::shared_ptr<TextureData> textureData;
		{
			// Wait for an event to say there is something in the queue
			std::unique_lock<std::mutex> lock(mMutex);
			mEvent.wait(lock);
			if (!mTextureDataQ.empty())
			{
				textureData = mTextureDataQ.front();
				mTextureDataQ.pop_front();
				mTextureDataLookup.erase(mTextureDataLookup.find(textureData.get()));
			}
		}
		// Queue has been released here but we might have a texture to process
		while (textureData)
		{
			textureData->load();

			// See if there is another item in the queue
			textureData = nullptr;
			std::unique_lock<std::mutex> lock(mMutex);
			if (!mTextureDataQ.empty())
			{
				textureData = mTextureDataQ.front();
				mTextureDataQ.pop_front();
				mTextureDataLookup.erase(mTextureDataLookup.find(textureData.get()));
			}
		}
	}
}

void TextureLoader::load(std::shared_ptr<TextureData> textureData)
{
	// Make sure it's not already loaded
	if (!textureData->isLoaded())
	{
		std::unique_lock<std::mutex> lock(mMutex);
		// Remove it from the queue if it is already there
		auto td = mTextureDataLookup.find(textureData.get());
		if (td != mTextureDataLookup.cend())
		{
			mTextureDataQ.erase((*td).second);
			mTextureDataLookup.erase(td);
		}

		// Put it on the start of the queue as we want the newly requested textures to load first
		mTextureDataQ.push_front(textureData);
		mTextureDataLookup[textureData.get()] = mTextureDataQ.cbegin();
		mEvent.notify_one();
	}
}

void TextureLoader::remove(std::shared_ptr<TextureData> textureData)
{
	// Just remove it from the queue so we don't attempt to load it
	std::unique_lock<std::mutex> lock(mMutex);
	auto td = mTextureDataLookup.find(textureData.get());
	if (td != mTextureDataLookup.cend())
	{
		mTextureDataQ.erase((*td).second);
		mTextureDataLookup.erase(td);
	}
}

size_t TextureLoader::getQueueSize()
{
	// Gets the amount of video memory that will be used once all textures in
	// the queue are loaded
	size_t mem = 0;
	std::unique_lock<std::mutex> lock(mMutex);
	for (auto tex : mTextureDataQ)
	{
		mem += tex->width() * tex->height() * 4;
	}
	return mem;
}
