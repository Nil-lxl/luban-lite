// 定义项目根路径
        const PROJECT_ROOT = '//docs/aicdoc/inputs';

        // 定义语言文件夹的映射
        const languageFolders = {
            'zh-cn': 'zh-cn', // 中文文件夹
            en: 'en'          // 英文文件夹
        };

        // 移除路径中的语言文件夹部分
        function removeLanguageFolder(path, folders) {
            const pathParts = path.split('/');
            if (pathParts.length > 1 && folders[pathParts[1]]) {
                // 跳过语言文件夹部分
                return '/' + pathParts.slice(2).join('/');
            }
            return path;
        }

        // 获取当前页面相对于项目根目录的路径（不包括语言文件夹）
        function getCurrentPagePath() {
            try {
                const currentPath = window.location.pathname;
                const rootIndex = currentPath.indexOf('/' + PROJECT_ROOT.split('/').pop());
                if (rootIndex!== -1) {
                    const relativePath = currentPath.slice(rootIndex + PROJECT_ROOT.split('/').pop().length + 1);
                    return removeLanguageFolder(relativePath, languageFolders);
                }
                return removeLanguageFolder(currentPath, languageFolders);
            } catch (error) {
                console.error('Error getting current page path:', error);
                return '/';
            }
        }

         // 处理路径中的重复语言文件夹
        function handleDuplicateLanguagePath(newLang, path) {
            if (newLang === 'zh-cn' && path.includes('/zh-cn/')) {
                return path.replace('/zh-cn/', '/');
            }
            return path;
        }

        // 构建新语言的页面 URL
        function buildNewPageUrl(newLang, currentPath) {
            const languageFolder = languageFolders[newLang];
            const processedPath = handleDuplicateLanguagePath(newLang, currentPath);
            return `${PROJECT_ROOT}/${languageFolder}${processedPath}`;
        }

        // 切换语言
        function changeLanguage(newLang) {
            const languageFolder = languageFolders[newLang];
            if (!languageFolder) {
                console.error('Unsupported language:', newLang);
                alert('Unsupported language: ' + newLang);
                return;
            }

            // 获取当前页面的路径（不包括语言文件夹）
            const currentPagePath = getCurrentPagePath();

            // 构建新语言的页面 URL
            const newPageUrl = buildNewPageUrl(newLang, currentPagePath);

            // 跳转到新语言的页面
            try {
                window.location.href = newPageUrl;
            } catch (error) {
                console.error('Error changing language:', error);
                alert('Error changing language. Please try again.');
            }
        }

        // 检测当前语言
        function detectCurrentLanguage() {
            try {
                const pathParts = window.location.pathname.split('/');
                if (pathParts.length > 1 && languageFolders[pathParts[1]]) {
                    return pathParts[1];
                }
                return 'zh-cn';
            } catch (error) {
                console.error('Error detecting current language:', error);
                return 'zh-cn';
            }
        }

        // 初始化语言选择器
        function updateLanguageSelector() {
            const currentLang = detectCurrentLanguage();
            const languageSelect = document.querySelector('.language-select');
            if (languageSelect) {
                languageSelect.value = currentLang;
            }
        }

        // 页面加载时初始化语言选择器
        document.addEventListener('DOMContentLoaded', function () {
            updateLanguageSelector();
        });

        // 切换搜索框的显示状态
        function toggleSearchBox() {
            const searchBox = document.getElementById('searchBox');
            searchBox.classList.toggle('show');
        }