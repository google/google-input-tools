# Copyright 2014 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re
def getProjects(content):
  projectRe = r'Project\("\{[\d\-A-Z]+?\}\"\) = \"[^()"]+?\", \"([^()"]+?).vcxproj\", \"(\{[\dA-Z\-]+?\})\"'
  result = re.findall(projectRe, content)
  return result

CONFIGS = '''
\tGlobalSection(SolutionConfigurationPlatforms) = preSolution
\t\tDebug|Mixed Platforms = Debug|Mixed Platforms
\t\tRelease|Mixed Platforms = Release|Mixed Platforms
\tEndGlobalSection
'''
if __name__ == '__main__':
    file = open('all.sln', 'r')
    content = file.read()
    file.close()
    projects = getProjects(content)
    config = '\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n'
    for project in projects:
      name = project[0]
      guid = project[1]
      if name.endswith('_x64'):
        config += '\t\t' + guid + '.Debug|Mixed Platforms.ActiveCfg = Debug|x64\n'
        config += '\t\t' + guid + '.Debug|Mixed Platforms.Build.0 = Debug|x64\n'
        config += '\t\t' + guid + '.Release|Mixed Platforms.ActiveCfg = Release|x64\n'
        config += '\t\t' + guid + '.Release|Mixed Platforms.Build.0 = Release|x64\n'
      else:
        config += '\t\t' + guid + '.Debug|Mixed Platforms.ActiveCfg = Debug|WIN32\n'
        config += '\t\t' + guid + '.Debug|Mixed Platforms.Build.0 = Debug|WIN32\n'
        config += '\t\t' + guid + '.Release|Mixed Platforms.ActiveCfg = Release|WIN32\n'
        config += '\t\t' + guid + '.Release|Mixed Platforms.Build.0 = Release|WIN32\n'
    config +='\tEndGlobalSection'
    configRE = re.compile(r'\tGlobalSection\(SolutionConfigurationPlatforms\) = preSolution.+?\tEndGlobalSection', re.S)
    content = configRE.sub(CONFIGS, content)
    configPostRE = re.compile(r'\tGlobalSection\(ProjectConfigurationPlatforms\) = postSolution.+?\tEndGlobalSection', re.S)
    content = configPostRE.sub(config, content)
    file = open('all.sln', 'w')
    file.write(content)
    file.close()

