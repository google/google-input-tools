import re
def getProjects(content):
  projectRe = r'Project\("\{[\d\-A-Z]+?\}\"\) = \"[^()"]+?\", \"([^()"]+?).vcxproj\", \"(\{[\dA-Z\-]+?\})\"'
  result = re.findall(projectRe, content)
  return result

CONFIG = '''
\tGlobalSection(SolutionConfigurationPlatforms) = preSolution
\t\tDebug|WINALL = Debug|WIN32
\t\tRelease|WINALL = Release|WIN32
\tEndGlobalSection
'''
if __name__ == '__main__':
    file = open('all.sln', 'r')
    content = file.read()
    file.close()
    projects = getProjects(content)
    config = '\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\t'
    for project in projects:
      name = project[0]
      guid = project[1]
      if name.endswith('_x64'):
        config += '\t\t' + guid + '.Debug|WINALL.ActiveCfg = Debug|x64\n'
        config += '\t\t' + guid + '.Debug|WINALL.Build.0 = Debug|x64\n'
        config += '\t\t' + guid + '.Release|WINALL.ActiveCfg = Release|x64\n'
        config += '\t\t' + guid + '.Release|WINALL.Build.0 = Release|x64\n'
      else:
        config += '\t\t' + guid + '.Debug|WINALL.ActiveCfg = Debug|WIN32\n'
        config += '\t\t' + guid + '.Debug|WINALL.Build.0 = Debug|WIN32\n'
        config += '\t\t' + guid + '.Release|WINALL.ActiveCfg = Release|WIN32\n'
        config += '\t\t' + guid + '.Release|WINALL.Build.0 = Release|WIN32\n'
    config +='\tEndGlobalSection'
    configRE = re.compile(r'\tGlobalSection\(SolutionConfigurationPlatforms\) = preSolution.+?\tEndGlobalSection', re.S)
    content = configRE.sub(CONFIG, content)
    configPostRE = re.compile(r'\tGlobalSection\(SolutionConfigurationPlatforms\) = postSolution.+?\tEndGlobalSection', re.S)
    content = configRE.sub(config, content)
    file = open('all.sln', 'w')
    file.write(content)
    file.close()

